#include "openminecraft/renderer/vk/om_renderer_layer_vk_testrenderer.hpp"

#include "glm/fwd.hpp"
#include "openminecraft/fontproc/om_font.hpp"
#include "openminecraft/renderer/common/basics/om_camera.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
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

    auto attaches = std::vector{AttachmentReference(0, ImageLayout::eColorAttachmentOptimal)};
    auto depthAtt = AttachmentReference(1, ImageLayout::eDepthStencilAttachmentOptimal);

    auto attachments = std::vector{
        AttachmentDescription({}, renderer->swapchainManager->format.format, SampleCountFlagBits::e1,
                              AttachmentLoadOp::eClear, AttachmentStoreOp::eStore, AttachmentLoadOp::eDontCare,
                              AttachmentStoreOp::eDontCare, ImageLayout::eUndefined, ImageLayout::ePresentSrcKHR),
        AttachmentDescription({}, Format::eD32Sfloat, SampleCountFlagBits::e1, AttachmentLoadOp::eClear,
                              AttachmentStoreOp::eDontCare, AttachmentLoadOp::eDontCare, AttachmentStoreOp::eDontCare,
                              ImageLayout::eUndefined, ImageLayout::eDepthStencilAttachmentOptimal)};
    auto subpasses =
        std::vector{SubpassDescription({}, PipelineBindPoint::eGraphics, nullptr, attaches, {}, &depthAtt)};
    auto depe = std::vector{SubpassDependency(
        VK_SUBPASS_EXTERNAL, 0,
        PipelineStageFlagBits::eColorAttachmentOutput | PipelineStageFlagBits::eEarlyFragmentTests,
        PipelineStageFlagBits::eColorAttachmentOutput | PipelineStageFlagBits::eEarlyFragmentTests, {},
        AccessFlagBits::eColorAttachmentRead | AccessFlagBits::eColorAttachmentWrite |
            AccessFlagBits::eDepthStencilAttachmentWrite)};

    renderPass = renderer->logicalDevice.createRenderPass(RenderPassCreateInfo({}, attachments, subpasses, depe),
                                                          renderer->allocator);

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
            vtxnew.push_back({{v.x, v.y, 0.0f}, {uvv, uvv2}});
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

    commandPool = renderer->logicalDevice.createCommandPool(CommandPoolCreateInfo({}, renderer->queueFamilyIndex.first),
                                                            renderer->allocator);

    const std::vector b = {
        DescriptorSetLayoutBinding(0, DescriptorType::eUniformBuffer, 1, ShaderStageFlagBits::eVertex)};
    descriptorSetLayouts.emplace_back(
        renderer->logicalDevice.createDescriptorSetLayout(DescriptorSetLayoutCreateInfo({}, b), renderer->allocator));

    const std::vector b2 = {
        DescriptorSetLayoutBinding(0, DescriptorType::eCombinedImageSampler, 1, ShaderStageFlagBits::eFragment)};
    descriptorSetLayouts.emplace_back(
        renderer->logicalDevice.createDescriptorSetLayout(DescriptorSetLayoutCreateInfo({}, b2), renderer->allocator));

    const std::vector a = {DescriptorPoolSize(DescriptorType::eUniformBuffer, renderer->framesInFlight),
                           DescriptorPoolSize(DescriptorType::eCombinedImageSampler, 1)};

    descriptorPool = renderer->logicalDevice.createDescriptorPool(
        DescriptorPoolCreateInfo(DescriptorPoolCreateFlagBits::eFreeDescriptorSet, renderer->framesInFlight + 1, a),
        renderer->allocator);
    descriptorSet = renderer->logicalDevice.allocateDescriptorSets(
        DescriptorSetAllocateInfo(descriptorPool, descriptorSetLayouts[0]))[0];

    const std::vector c = {DescriptorBufferInfo(reinterpret_cast<OMRendererBufferVk *>(uniformBuffer)->buffer, 0,
                                                sizeof(UniformStructure))};
    renderer->logicalDevice.updateDescriptorSets(
        WriteDescriptorSet(descriptorSet, 0, 0, DescriptorType::eUniformBuffer, {}, c), nullptr);

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

    {
        auto prop = renderer->physicalDevice.getProperties();
        auto fea = renderer->physicalDevice.getFeatures();

        textureSampler = renderer->logicalDevice.createSampler(
            SamplerCreateInfo({}, Filter::eLinear, Filter::eLinear, SamplerMipmapMode::eLinear,
                              SamplerAddressMode::eRepeat, SamplerAddressMode::eRepeat, SamplerAddressMode::eRepeat,
                              0.0f, fea.samplerAnisotropy, prop.limits.maxSamplerAnisotropy, false, CompareOp::eAlways,
                              0.0f, 0.0f, BorderColor::eIntOpaqueBlack, false),
            renderer->allocator);
    }

    combinedDescriptorSet = renderer->logicalDevice.allocateDescriptorSets(
        DescriptorSetAllocateInfo(descriptorPool, descriptorSetLayouts[1]))[0];

    const auto cc =
        DescriptorImageInfo(textureSampler, reinterpret_cast<OMRendererTextureVk *>(textureImage)->imageView,
                            ImageLayout::eShaderReadOnlyOptimal);
    renderer->logicalDevice.updateDescriptorSets(
        WriteDescriptorSet(combinedDescriptorSet, 0, 0, DescriptorType::eCombinedImageSampler, cc), nullptr);

    pipelineLayout = renderer->logicalDevice.createPipelineLayout(PipelineLayoutCreateInfo({}, descriptorSetLayouts),
                                                                  renderer->allocator);

    {
        auto shaders =
            std::vector{PipelineShaderStageCreateInfo(
                            {}, ShaderStageFlagBits::eVertex,
                            renderer->logicalDevice.createShaderModule(
                                ShaderModuleCreateInfo({}, vtxShader->data.size(),
                                                       reinterpret_cast<const uint32_t *>(vtxShader->data.data())),
                                renderer->allocator),
                            "main"),
                        PipelineShaderStageCreateInfo(
                            {}, ShaderStageFlagBits::eFragment,
                            renderer->logicalDevice.createShaderModule(
                                ShaderModuleCreateInfo({}, frgShader->data.size(),
                                                       reinterpret_cast<const uint32_t *>(frgShader->data.data())),
                                renderer->allocator),
                            "main")};

        const std::vector bi = {VertexInputBindingDescription(0, (3 + 2) * sizeof(float), VertexInputRate::eVertex)};
        const std::vector ad = {VertexInputAttributeDescription(0, 0, Format::eR32G32B32Sfloat, 0),
                                VertexInputAttributeDescription(1, 0, Format::eR32G32Sfloat, 3 * sizeof(float))};

        auto vertexInput = PipelineVertexInputStateCreateInfo({}, bi, ad);
        auto inputAssembly = PipelineInputAssemblyStateCreateInfo({}, PrimitiveTopology::eTriangleList, false);

        auto rasterization =
            PipelineRasterizationStateCreateInfo({}, false, false, PolygonMode::eFill, CullModeFlagBits::eNone,
                                                 FrontFace::eCounterClockwise, true, 0, 0, 0, 1);
        auto multisample = PipelineMultisampleStateCreateInfo({}, SampleCountFlagBits::e1, false);
        auto viewportState = PipelineViewportStateCreateInfo({}, 1, nullptr, 1, nullptr);
        const std::vector attc = {
            PipelineColorBlendAttachmentState(false, {}, {}, {}, {}, {}, {},
                                              ColorComponentFlagBits::eA | ColorComponentFlagBits::eR |
                                                  ColorComponentFlagBits::eG | ColorComponentFlagBits::eB)};
        auto colorblend =
            PipelineColorBlendStateCreateInfo({}, true, LogicOp::eCopy, attc, std::array{0.f, 0.f, 0.f, 0.f});

        auto depthStencil =
            PipelineDepthStencilStateCreateInfo({}, true, true, CompareOp::eLess, true, true, {}, {}, 0.0f, 1.0f);
        std::vector<DynamicState> states = {DynamicState::eScissor, DynamicState::eViewport};
        auto dynamicState = PipelineDynamicStateCreateInfo({}, 2, states.data());

        auto result = renderer->logicalDevice.createGraphicsPipeline(
            {},
            GraphicsPipelineCreateInfo({}, shaders, &vertexInput, &inputAssembly, {}, &viewportState, &rasterization,
                                       &multisample, &depthStencil, &colorblend, &dynamicState, pipelineLayout,
                                       renderPass, 0, {}, -1),
            renderer->allocator);
        if (result.result != Result::eSuccess)
        {
            throw SystemError(result.result);
        }
        pipeline = result.value;

        for (auto sd : shaders)
        {
            renderer->logicalDevice.destroyShaderModule(sd.module, renderer->allocator);
        }
    }

    {
        intermediateBuffer =
            renderer->logicalDevice.allocateCommandBuffers({commandPool, CommandBufferLevel::eSecondary, 1})[0];
        auto ii = CommandBufferInheritanceInfo(renderPass, 0);
        intermediateBuffer.begin(
            {CommandBufferUsageFlagBits::eSimultaneousUse | CommandBufferUsageFlagBits::eRenderPassContinue, &ii});
        intermediateBuffer.setViewport(0,
                                       {Viewport(0, 0, static_cast<float>(renderer->swapchainManager->extent.width),
                                                 static_cast<float>(renderer->swapchainManager->extent.height), 0, 1)});
        intermediateBuffer.setScissor(0, {Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent)});
        intermediateBuffer.bindPipeline(PipelineBindPoint::eGraphics, pipeline);
        intermediateBuffer.bindDescriptorSets(PipelineBindPoint::eGraphics, pipelineLayout, 0,
                                              std::vector{descriptorSet, combinedDescriptorSet}, nullptr);
        intermediateBuffer.bindVertexBuffers(
            0, std::vector{reinterpret_cast<OMRendererBufferVk *>(vertexBuffer)->buffer}, std::vector<DeviceSize>{0});
        intermediateBuffer.bindIndexBuffer(reinterpret_cast<OMRendererBufferVk *>(indexBuffer)->buffer, 0,
                                           IndexType::eUint32);
        intermediateBuffer.drawIndexed(vertexCount, 1, 0, 0, 0);
        intermediateBuffer.end();
    }

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
    if (!commandBuffers.empty())
    {
        renderer->logicalDevice.freeCommandBuffers(commandPool, commandBuffers);
    }
    for (auto framebuffer : framebuffers)
    {
        renderer->logicalDevice.destroyFramebuffer(framebuffer, renderer->allocator);
    }
    commandBuffers.clear();
    framebuffers.clear();

    // these old resources need to be cleaned
    if (!firstTime)
    {
        delete depthBuffer;
    }

    {
        depthBuffer = renderer->allocateTexture(renderer->swapchainManager->extent.width,
                                                renderer->swapchainManager->extent.height, common::Dim2, common::Depth);
    }

    for (auto img : renderer->swapchainManager->swapchainImageViews)
    {
        const std::vector ii = {img, reinterpret_cast<OMRendererTextureVk *>(depthBuffer)->imageView}; // depthImageView
        framebuffers.push_back(renderer->logicalDevice.createFramebuffer(
            FramebufferCreateInfo({}, renderPass, ii, renderer->swapchainManager->extent.width,
                                  renderer->swapchainManager->extent.height, 1),
            renderer->allocator));
    }

    int i = 0;
    for (auto framebuffer : framebuffers)
    {
        auto commandBuffer = renderer->logicalDevice.allocateCommandBuffers(
            CommandBufferAllocateInfo(commandPool, CommandBufferLevel::ePrimary, 1))[0];

        commandBuffer.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eSimultaneousUse));
        std::vector test = {ClearValue({0, 0, 0, 0}), ClearValue({1.0f, 0})};
        commandBuffer.beginRenderPass(RenderPassBeginInfo(renderPass, framebuffer,
                                                          Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent),
                                                          test),
                                      SubpassContents::eSecondaryCommandBuffers);
        /*commandBuffer.setViewport(0, {Viewport(0, 0, static_cast<float>(renderer->swapchainManager->extent.width),
                                               static_cast<float>(renderer->swapchainManager->extent.height), 0, 1)});
        commandBuffer.setScissor(0, {Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent)});
        commandBuffer.bindPipeline(PipelineBindPoint::eGraphics, pipeline);
        commandBuffer.bindDescriptorSets(PipelineBindPoint::eGraphics, pipelineLayout, 0,
                                         std::vector{descriptorSet, combinedDescriptorSet}, nullptr);
        commandBuffer.bindVertexBuffers(0, std::vector{reinterpret_cast<OMRendererBufferVk *>(vertexBuffer)->buffer},
                                        std::vector<DeviceSize>{0});
        commandBuffer.bindIndexBuffer(reinterpret_cast<OMRendererBufferVk *>(indexBuffer)->buffer, 0,
                                      IndexType::eUint32);
        commandBuffer.drawIndexed(vertexCount, 1, 0, 0, 0);*/
        commandBuffer.executeCommands(intermediateBuffer);
        commandBuffer.endRenderPass();
        commandBuffer.end();

        commandBuffers.push_back(commandBuffer);
        i++;
    }
}
void OMTestRenderer::destroy()
{
    delete depthBuffer;

    renderer->logicalDevice.freeDescriptorSets(descriptorPool, combinedDescriptorSet);
    renderer->logicalDevice.destroySampler(textureSampler, renderer->allocator);
    delete textureImage;
    renderer->logicalDevice.freeDescriptorSets(descriptorPool, descriptorSet);
    renderer->logicalDevice.destroyDescriptorPool(descriptorPool, renderer->allocator);
    delete uniformBuffer;
    for (auto l : descriptorSetLayouts)
    {
        renderer->logicalDevice.destroyDescriptorSetLayout(l, renderer->allocator);
    }
    delete vertexBuffer;
    delete indexBuffer;
    renderer->logicalDevice.freeCommandBuffers(commandPool, intermediateBuffer);
    renderer->logicalDevice.freeCommandBuffers(commandPool, commandBuffers);
    renderer->logicalDevice.destroyCommandPool(commandPool, renderer->allocator);
    renderer->logicalDevice.destroyPipeline(pipeline, renderer->allocator);
    renderer->logicalDevice.destroyPipelineLayout(pipelineLayout, renderer->allocator);
    for (auto framebuffer : framebuffers)
    {
        renderer->logicalDevice.destroyFramebuffer(framebuffer, renderer->allocator);
    }
    renderer->logicalDevice.destroyRenderPass(renderPass, renderer->allocator);
}

} // namespace openminecraft::renderer::vk::test
