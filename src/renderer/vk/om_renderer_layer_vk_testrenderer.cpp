#include "openminecraft/renderer/vk/om_renderer_layer_vk_testrenderer.hpp"

#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "tiny_obj_loader.h"

#include <fstream>
#include <glm/glm.hpp>
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

        std::vector<VertexPart> vtxnew;
        std::vector<uint32_t> indices;

        std::map<VertexPart, uint32_t> uniqueVertices;

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                VertexPart prt = {{attrib.vertices[3 * index.vertex_index + 0],
                                   attrib.vertices[3 * index.vertex_index + 1],
                                   attrib.vertices[3 * index.vertex_index + 2]},
                                  {attrib.texcoords[2 * index.texcoord_index + 0],
                                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]}};

                if (!uniqueVertices.count(prt))
                {
                    uniqueVertices[prt] = static_cast<uint32_t>(vtxnew.size());
                    vtxnew.push_back(prt);
                }
                indices.push_back(uniqueVertices[prt]);
            }
        }

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

    const auto cc = DescriptorImageInfo(textureSampler, reinterpret_cast<OMRendererTextureVk *>(textureImage)->imageView, ImageLayout::eShaderReadOnlyOptimal);
    renderer->logicalDevice.updateDescriptorSets(
        WriteDescriptorSet(combinedDescriptorSet, 0, 0, DescriptorType::eCombinedImageSampler, cc), nullptr);

    OMTestRenderer::reinit();

    firstTime = false;
}

void OMTestRenderer::updateUniform()
{
    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front = glm::normalize(front);

    UniformStructure ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.model *= glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(m_cameraPos, m_cameraPos + front, m_cameraUp);
    ubo.proj = glm::perspective(glm::radians(70.0f),
                                static_cast<float>(renderer->swapchainManager->extent.width) /
                                    static_cast<float>(renderer->swapchainManager->extent.height),
                                0.1f, 20.0f);
    ubo.proj[1][1] *= -1;

    uniformBuffer->updateData(&ubo);
}

void OMTestRenderer::mouseOffset(float dx, float dy)
{
    m_pitch -= dy * m_cameraRotateSpeed;
    m_yaw += dx * m_cameraRotateSpeed;

    if (m_yaw < 0.0f)
        m_yaw += 360.0f;
    m_yaw = std::fmod(m_yaw + 180.0f, 360.0f);
    m_yaw -= 180.0f;

    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;
}

void OMTestRenderer::keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float>(currentTime - startTime).count();
    startTime = currentTime;

    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    // front.y = std::sin(glm::radians(m_pitch));
    front.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front = glm::normalize(front);
    if (w)
    {
        m_cameraPos += front * m_cameraMoveSpeed * time;
    }
    if (s)
    {
        m_cameraPos -= front * m_cameraMoveSpeed * time;
    }
    if (a)
    {
        m_cameraPos -= glm::normalize(glm::cross(front, m_cameraUp)) * m_cameraMoveSpeed * time;
    }
    if (d)
    {
        m_cameraPos += glm::normalize(glm::cross(front, m_cameraUp)) * m_cameraMoveSpeed * time;
    }
    if (sp)
    {
        m_cameraPos += m_cameraUp * m_cameraMoveSpeed * time;
    }
    if (lsh)
    {
        m_cameraPos -= m_cameraUp * m_cameraMoveSpeed * time;
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

    if (!firstTime)
    {
        renderer->logicalDevice.freeMemory(depthImageMemory, renderer->allocator);
        renderer->logicalDevice.destroyImageView(depthImageView, renderer->allocator);
        renderer->logicalDevice.destroyImage(depthImage, renderer->allocator);
        renderer->logicalDevice.destroyPipeline(pipeline, renderer->allocator);
        renderer->logicalDevice.destroyPipelineLayout(pipelineLayout, renderer->allocator);
    }

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

        const std::vector vp = {Viewport(0, 0, static_cast<float>(renderer->swapchainManager->extent.width),
                                         static_cast<float>(renderer->swapchainManager->extent.height), 0, 1)};
        const std::vector scis = {Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent)};
        auto viewportState = PipelineViewportStateCreateInfo({}, vp, scis);
        auto rasterization =
            PipelineRasterizationStateCreateInfo({}, false, false, PolygonMode::eFill, CullModeFlagBits::eNone,
                                                 FrontFace::eCounterClockwise, true, 0, 0, 0, 1);
        auto multisample = PipelineMultisampleStateCreateInfo({}, SampleCountFlagBits::e1, false);

        const std::vector attc = {
            PipelineColorBlendAttachmentState(false, {}, {}, {}, {}, {}, {},
                                              ColorComponentFlagBits::eA | ColorComponentFlagBits::eR |
                                                  ColorComponentFlagBits::eG | ColorComponentFlagBits::eB)};
        auto colorblend =
            PipelineColorBlendStateCreateInfo({}, true, LogicOp::eCopy, attc, std::array{0.f, 0.f, 0.f, 0.f});

        auto depthStencil =
            PipelineDepthStencilStateCreateInfo({}, true, true, CompareOp::eLess, true, true, {}, {}, 0.0f, 1.0f);

        auto result = renderer->logicalDevice.createGraphicsPipeline(
            {},
            GraphicsPipelineCreateInfo({}, shaders, &vertexInput, &inputAssembly, {}, &viewportState, &rasterization,
                                       &multisample, &depthStencil, &colorblend, {}, pipelineLayout, renderPass, 0, {},
                                       -1),
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
        auto prop = renderer->physicalDevice.getMemoryProperties();
        depthImage = renderer->logicalDevice.createImage(
            ImageCreateInfo(
                {}, ImageType::e2D, Format::eD32Sfloat,
                Extent3D(renderer->swapchainManager->extent.width, renderer->swapchainManager->extent.height, 1), 1, 1,
                SampleCountFlagBits::e1, ImageTiling::eOptimal, ImageUsageFlagBits::eDepthStencilAttachment,
                SharingMode::eExclusive, {}, ImageLayout::eUndefined),
            renderer->allocator);
        auto req = renderer->logicalDevice.getImageMemoryRequirements(depthImage);
        depthImageMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(req.size,
                               findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eDeviceLocal, prop)),
            renderer->allocator);
        renderer->logicalDevice.bindImageMemory(depthImage, depthImageMemory, 0);

        depthImageView = renderer->logicalDevice.createImageView(
            ImageViewCreateInfo({}, depthImage, ImageViewType::e2D, Format::eD32Sfloat, {},
                                ImageSubresourceRange(ImageAspectFlagBits::eDepth, 0, 1, 0, 1)),
            renderer->allocator);
    }

    for (auto img : renderer->swapchainManager->swapchainImageViews)
    {
        const std::vector ii = {img, depthImageView};
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
        std::vector test = {ClearValue({55, 55, 55, 55}), ClearValue({1.0f, 0})};
        commandBuffer.beginRenderPass(RenderPassBeginInfo(renderPass, framebuffer,
                                                          Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent),
                                                          test),
                                      SubpassContents::eInline);
        commandBuffer.bindPipeline(PipelineBindPoint::eGraphics, pipeline);
        commandBuffer.bindDescriptorSets(PipelineBindPoint::eGraphics, pipelineLayout, 0,
                                         std::vector{descriptorSet, combinedDescriptorSet}, nullptr);
        commandBuffer.bindVertexBuffers(0, std::vector{reinterpret_cast<OMRendererBufferVk *>(vertexBuffer)->buffer},
                                        std::vector<DeviceSize>{0});
        commandBuffer.bindIndexBuffer(reinterpret_cast<OMRendererBufferVk *>(indexBuffer)->buffer, 0,
                                      IndexType::eUint32);
        commandBuffer.drawIndexed(vertexCount, 1, 0, 0, 0);
        commandBuffer.endRenderPass();
        commandBuffer.end();

        commandBuffers.push_back(commandBuffer);
        i++;
    }
}
void OMTestRenderer::destroy()
{
    renderer->logicalDevice.freeMemory(depthImageMemory, renderer->allocator);
    renderer->logicalDevice.destroyImageView(depthImageView, renderer->allocator);
    renderer->logicalDevice.destroyImage(depthImage, renderer->allocator);

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
