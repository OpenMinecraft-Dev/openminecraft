#include "openminecraft/renderer/vk/om_renderer_layer_vk_testrenderer.hpp"

#include "glm/fwd.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/basics/om_vertex_format.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_pipeline.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_rendertarget.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_task.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "tiny_obj_loader.h"
#include "vulkan/vulkan.hpp"

#include <chrono>
#include <glm/glm.hpp>
#include <random>
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
#include "openminecraft/binary/om_bin_hash.hpp"
#include "openminecraft/io/om_io_utils.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"

#include <stb_image.h>

using namespace ::vk;

namespace openminecraft::renderer::vk::test
{
uint32_t findMemoryType(uint32_t typeFilter, MemoryPropertyFlags properties,
                        PhysicalDeviceMemoryProperties &memProperties)
{
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    return 0;
}

OMTestRenderer::OMTestRenderer(OMRendererVk *renderer) : renderer(renderer), logger("OMTestRenderer", this)
{
    camera = std::make_shared<common::basics::OMCamera>(renderer, m_cameraPos, m_yaw, m_pitch);
    {
        auto target = vfs::fsfetch("/bootassets/openminecraft-renderer/shaders/simple.frag.glsl");
        common::OMShader shader(common::GLSLSource, io::readOnce(target.get()), "simple.frag.glsl", "main",
                                common::Fragment);
        frgShader = shader.convertTo(common::SPIRVBinary);
    }

    {
        auto target = vfs::fsfetch("/bootassets/openminecraft-renderer/shaders/simple.vert.glsl");
        common::OMShader shader(common::GLSLSource, io::readOnce(target.get()), "simple.vert.glsl", "main",
                                common::Vertex);
        vtxShader = shader.convertTo(common::SPIRVBinary);
    }

    {
        class VertexPart
        {
            glm::vec3 pos;
            glm::vec2 textureUV;

          public:
            VertexPart(glm::vec3 p, glm::vec2 uv) : pos(p), textureUV(uv)
            {
            }

            bool operator<(const VertexPart &other) const
            {
                return std::memcmp(&other, this, sizeof(VertexPart)) < 0;
            }
        };

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        auto strr = vfs::fsfetch("/bootassets/openminecraft-renderer/models/viking_room.obj");

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, strr.get()))
        {
            throw std::runtime_error("Warn: " + warn + "\nError: " + err);
        }

        auto iff = vfs::fsfetch("/bootassets/openminecraft-boot/font/StarRailFont.ttf");
        auto f = new fontproc::OMFont(*iff.get());
        auto ppo = f->buildBasicPolygon('@');
        delete f;

        std::vector<VertexPart> vtxnew = {{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
                                          {{1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, {{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
                                          {{0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}, {{0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
                                          {{1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}, {{1.0f, 1.0f, 0.0f}, {1.0f, 0.0f}}};
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4};

        vtxnew.clear();
        indices.clear();

        std::random_device dev;
        std::ranlux48 eng(dev());
        std::uniform_real_distribution<> dist(0.0f, 1.0f);

        auto uvv = 0.0f;
        auto uvv2 = 0.0f;
        auto iid = 0;

        for (auto &v : ppo->vertices)
        {
            if (!iid)
            {
                uvv = dist(eng);
                uvv2 = dist(eng);
            }
            vtxnew.push_back({{v.x, v.y, 0.0f}, {v.x, v.y}});
            iid++;
            iid = iid % 3;
        }

        for (auto i : ppo->indices)
        {
            indices.push_back(i);
        }

        /*std::map<VertexPart, uint32_t> uniqueVertices;

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                VertexPart prt = {{
                                      attrib.vertices[3 * index.vertex_index + 0],
                                      attrib.vertices[3 * index.vertex_index + 2],
                                      attrib.vertices[3 * index.vertex_index + 1],
                                  },
                                  {attrib.texcoords[2 * index.texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}};

                if (!uniqueVertices.count(prt))
                {
                    uniqueVertices[prt] = static_cast<uint32_t>(vtxnew.size());
                    vtxnew.push_back(prt);
                }
                indices.push_back(uniqueVertices[prt]);
            }
        }*/

        auto siz = vtxnew.size() * sizeof(VertexPart);

        vertexBuffer = renderer->allocateBuffer(common::VertexData, siz);
        vertexBuffer->updateData(vtxnew.data());

        siz = indices.size() * sizeof(uint32_t);
        indexBuffer = renderer->allocateBuffer(common::VertexIndex, siz);
        indexBuffer->updateData(indices.data());

        vertexCount = indices.size();
    }

    uniformBuffer = renderer->allocateBuffer(common::Uniform, sizeof(UniformStructure));

    {
        int texWidth, texHeight, texChannels;

        auto imgraw = vfs::fsfetch("/bootassets/openminecraft-renderer/texture/viking_room.png");
        auto tex = io::readOnce(imgraw.get());

        stbi_uc *pixels =
            stbi_load_from_memory(tex.data(), tex.size(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels)
            throw std::runtime_error("failed to load texture image!");

        textureImage = renderer->allocateTexture(texWidth, texHeight, common::Dim2, common::ColorRgba);
        textureImage->updateData(pixels);

        stbi_image_free(pixels);
    }

    pipeline = renderer->createPipeline();
    pipeline->appendInput(common::OMRendererPipelineInputType::UniformBuffer);
    pipeline->appendInput(common::OMRendererPipelineInputType::ImageSampler);
    pipeline->bindOutput(renderer->defaultTarget);
    pipeline->attachShader(frgShader);
    pipeline->attachShader(vtxShader);

    common::basics::OMVertexFormat format;
    format.appendPart("position", common::basics::Vec3f);
    format.appendPart("textureUV", common::basics::Vec2f);
    format.nextGroup();
    format.decideStruct();
    format.debugState();

    pipeline->vertexFormat(format);
    pipeline->build();

    pipeline->bindInput(0, uniformBuffer);
    pipeline->bindInput(1, textureImage);

    OMTestRenderer::reinit();

    firstTime = false;
}

void OMTestRenderer::updateUniform()
{
    UniformStructure ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = camera->fetchViewMat();
    ubo.proj = camera->fetchProjMat();
    // vulkan only!
    ubo.proj *= glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));

    uniformBuffer->updateData(&ubo);
}

void OMTestRenderer::mouseOffset(float dx, float dy)
{
    camera->modPitch(-dy * m_cameraRotateSpeed);
    camera->modYaw(dx * m_cameraRotateSpeed);
}

void OMTestRenderer::keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float>(currentTime - startTime).count();
    startTime = currentTime;

    if (w)
    {
        camera->moveCamera(common::basics::Forward, m_cameraMoveSpeed * time);
    }
    if (s)
    {
        camera->moveCamera(common::basics::Back, m_cameraMoveSpeed * time);
    }
    if (a)
    {
        camera->moveCamera(common::basics::Left, m_cameraMoveSpeed * time);
    }
    if (d)
    {
        camera->moveCamera(common::basics::Right, m_cameraMoveSpeed * time);
    }
    if (sp)
    {
        camera->moveCamera(common::basics::Up, m_cameraMoveSpeed * time);
    }
    if (lsh)
    {
        camera->moveCamera(common::basics::Down, m_cameraMoveSpeed * time);
    }
}

void OMTestRenderer::reinit()
{
    auto task = renderer->createTask();
    task->bindTarget(renderer->defaultTarget);
    task->bindPipeline(pipeline);
    task->bindVertexBuffer({vertexBuffer});
    task->bindIndexBuffer(indexBuffer);
    task->draw(vertexCount);
    task->finish();

    renderer->attachTask(task);
}
void OMTestRenderer::destroy()
{
    delete pipeline;
    delete textureImage;
    delete uniformBuffer;
    delete vertexBuffer;
    delete indexBuffer;
}

} // namespace openminecraft::renderer::vk::test
