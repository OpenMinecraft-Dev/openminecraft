#include "openminecraft/renderer/vk/om_renderer_layer_vk_testrenderer.hpp"

#include "glm/fwd.hpp"
#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"
#include "tiny_obj_loader.h"

#include <fstream>
#include <glm/glm.hpp>
#include <vulkan/vulkan_core.h>

#define STB_IMAGE_IMPLEMENTATION
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
        target->seekg(0, target->end);
        auto length = target->tellg();
        target->seekg(0, target->beg);
        std::vector<uint8_t> data(length);
        target->read((char *)data.data(), length);

        common::OMShader shader(common::GLSLSource, data, "simple.frag.glsl", "main", common::Fragment);
        frgShader = shader.convertTo(common::SPIRVBinary);
    }

    {
        auto target = vfs::fsfetch("/bootassets/openminecraft-renderer/shaders/simple.vert.glsl");
        target->seekg(0, target->end);
        auto length = target->tellg();
        target->seekg(0, target->beg);
        std::vector<uint8_t> data(length);
        target->read((char *)data.data(), length);

        common::OMShader shader(common::GLSLSource, data, "simple.vert.glsl", "main", common::Vertex);
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

    auto prop = renderer->physicalDevice.getMemoryProperties();
    {
        class VertexPart
        {
            glm::vec3 pos;
            glm::vec2 textureUV;

          public:
            VertexPart(glm::vec3 p, glm::vec2 uv) : pos(p), textureUV(uv) {};

            bool operator<(const VertexPart &other) const
            {
                return std::tie(pos.x, pos.y, pos.z, textureUV.x, textureUV.y) <
                       std::tie(other.pos.x, other.pos.y, other.pos.z, other.textureUV.x, other.textureUV.y);
            }
        };

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, "/bridge/models/objs/viking_room.obj"))
        {
            throw std::runtime_error(warn + err);
        }

        std::vector<VertexPart> vtxnew;
        std::vector<uint32_t> indices;

        std::map<VertexPart, uint32_t> uniqueVertices;

        for (const auto &shape : shapes)
        {
            for (const auto &index : shape.mesh.indices)
            {
                auto idx = vtxnew.size();
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
        vertexBuffer = renderer->logicalDevice.createBuffer(
            BufferCreateInfo({}, siz, BufferUsageFlagBits::eVertexBuffer, SharingMode::eExclusive),
            renderer->allocator);

        auto req = renderer->logicalDevice.getBufferMemoryRequirements(vertexBuffer);

        vertexBufferMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(
                req.size,
                findMemoryType(req.memoryTypeBits,
                               MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent, prop)),
            renderer->allocator);

        renderer->logicalDevice.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

        auto r = renderer->logicalDevice.mapMemory(vertexBufferMemory, 0, siz);

        std::memcpy(r, vtxnew.data(), siz);

        renderer->logicalDevice.unmapMemory(vertexBufferMemory);

        siz = indices.size() * sizeof(uint32_t);
        indexBuffer = renderer->logicalDevice.createBuffer(
            BufferCreateInfo({}, siz, BufferUsageFlagBits::eIndexBuffer, SharingMode::eExclusive), renderer->allocator);

        req = renderer->logicalDevice.getBufferMemoryRequirements(indexBuffer);
        indexBufferMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(
                req.size,
                findMemoryType(req.memoryTypeBits,
                               MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent, prop)),
            renderer->allocator);

        renderer->logicalDevice.bindBufferMemory(indexBuffer, indexBufferMemory, 0);

        r = renderer->logicalDevice.mapMemory(indexBufferMemory, 0, siz);

        std::memcpy(r, indices.data(), siz);

        renderer->logicalDevice.unmapMemory(indexBufferMemory);

        vertexCount = indices.size();
    }

    {
        auto siz = sizeof(UniformStructure);
        uniformBuffer = renderer->logicalDevice.createBuffer(
            BufferCreateInfo({}, siz, BufferUsageFlagBits::eUniformBuffer, SharingMode::eExclusive),
            renderer->allocator);
        auto req = renderer->logicalDevice.getBufferMemoryRequirements(uniformBuffer);
        uniformBufferMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(
                req.size,
                findMemoryType(req.memoryTypeBits,
                               MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent, prop)),
            renderer->allocator);
        renderer->logicalDevice.bindBufferMemory(uniformBuffer, uniformBufferMemory, 0);

        mappedUniformBuffer = renderer->logicalDevice.mapMemory(uniformBufferMemory, 0, siz);
    }

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

    const std::vector c = {DescriptorBufferInfo(uniformBuffer, 0, sizeof(UniformStructure))};
    renderer->logicalDevice.updateDescriptorSets(
        WriteDescriptorSet(descriptorSet, 0, 0, DescriptorType::eUniformBuffer, {}, c), nullptr);

    {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels =
            stbi_load("/bridge/models/objs/viking_room.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (!pixels)
            throw std::runtime_error("failed to load texture image!");
        auto imageSize = texWidth * texHeight * 4;

        stagingBuffer = renderer->logicalDevice.createBuffer(
            BufferCreateInfo({}, imageSize, BufferUsageFlagBits::eTransferSrc, SharingMode::eExclusive),
            renderer->allocator);
        auto req = renderer->logicalDevice.getBufferMemoryRequirements(stagingBuffer);
        stagingBufferMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(
                req.size,
                findMemoryType(req.memoryTypeBits,
                               MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent, prop)),
            renderer->allocator);
        renderer->logicalDevice.bindBufferMemory(stagingBuffer, stagingBufferMemory, 0);

        auto r = renderer->logicalDevice.mapMemory(stagingBufferMemory, 0, imageSize);

        std::memcpy(r, pixels, imageSize);

        renderer->logicalDevice.unmapMemory(stagingBufferMemory);

        stbi_image_free(pixels);

        textureImage = renderer->logicalDevice.createImage(
            ImageCreateInfo({}, ImageType::e2D, Format::eR8G8B8A8Srgb, Extent3D(texWidth, texHeight, 1), 1, 1,
                            SampleCountFlagBits::e1, ImageTiling::eOptimal,
                            ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled, SharingMode::eExclusive,
                            {}, ImageLayout::eUndefined),
            renderer->allocator);

        req = renderer->logicalDevice.getImageMemoryRequirements(textureImage);
        imageMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(req.size,
                               findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eDeviceLocal, prop)),
            renderer->allocator);

        renderer->logicalDevice.bindImageMemory(textureImage, imageMemory, 0);

        transitionImageLayout(textureImage, Format::eR8G8B8A8Srgb, ImageLayout::eUndefined,
                              ImageLayout::eTransferDstOptimal);
        copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
        transitionImageLayout(textureImage, Format::eR8G8B8A8Srgb, ImageLayout::eTransferDstOptimal,
                              ImageLayout::eShaderReadOnlyOptimal);

        textureImageView = renderer->logicalDevice.createImageView(
            ImageViewCreateInfo({}, textureImage, ImageViewType::e2D, Format::eR8G8B8A8Srgb, {},
                                ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1)),
            renderer->allocator);
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

    const auto cc = DescriptorImageInfo(textureSampler, textureImageView, ImageLayout::eShaderReadOnlyOptimal);
    renderer->logicalDevice.updateDescriptorSets(
        WriteDescriptorSet(combinedDescriptorSet, 0, 0, DescriptorType::eCombinedImageSampler, cc), nullptr);

    OMTestRenderer::reinit();

    firstTime = false;
}

CommandBuffer OMTestRenderer::beginSingleTimeCommands()
{
    auto cmdBuff = renderer->logicalDevice.allocateCommandBuffers(
        CommandBufferAllocateInfo(commandPool, CommandBufferLevel::ePrimary, 1))[0];
    cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));
    return cmdBuff;
}
void OMTestRenderer::endSingleTimeCommands(CommandBuffer cmdBuff)
{
    cmdBuff.end();

    renderer->queues.first.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff));
    renderer->queues.first.waitIdle();

    renderer->logicalDevice.freeCommandBuffers(commandPool, 1, &cmdBuff);
}

void OMTestRenderer::copyBufferToImage(Buffer buffer, Image image, uint32_t width, uint32_t height)
{
    auto cmd = beginSingleTimeCommands();
    cmd.copyBufferToImage(buffer, image, ImageLayout::eTransferDstOptimal,
                          BufferImageCopy(0, 0, 0, ImageSubresourceLayers(ImageAspectFlagBits::eColor, 0, 0, 1),
                                          Offset3D(0, 0, 0), Extent3D(width, height, 1)));
    endSingleTimeCommands(cmd);
}

void OMTestRenderer::copyBuffer(Buffer srcBuff, Buffer dstBuff, DeviceSize size)
{
    auto cmd = beginSingleTimeCommands();
    cmd.copyBuffer(srcBuff, dstBuff, BufferCopy({}, {}, size));
    endSingleTimeCommands(cmd);
}

void OMTestRenderer::transitionImageLayout(Image image, Format format, ImageLayout oldLayout, ImageLayout newLayout)
{
    auto barrier = ImageMemoryBarrier({}, {}, oldLayout, newLayout, QueueFamilyIgnored, QueueFamilyIgnored, image,
                                      ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1));

    PipelineStageFlagBits sourceStage, destinationStage;

    if (oldLayout == ImageLayout::eUndefined && newLayout == ImageLayout::eTransferDstOptimal)
    {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = AccessFlagBits::eTransferWrite;

        sourceStage = PipelineStageFlagBits::eTopOfPipe;
        destinationStage = PipelineStageFlagBits::eTransfer;
    }
    else if (oldLayout == ImageLayout::eTransferDstOptimal && newLayout == ImageLayout::eShaderReadOnlyOptimal)
    {
        barrier.srcAccessMask = AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = AccessFlagBits::eShaderRead;

        sourceStage = PipelineStageFlagBits::eTransfer;
        destinationStage = PipelineStageFlagBits::eFragmentShader;
    }
    else
    {
        throw std::invalid_argument("unsupported layout transition!");
    }

    auto cmd = beginSingleTimeCommands();
    cmd.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);
    endSingleTimeCommands(cmd);
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

    std::memcpy(mappedUniformBuffer, &ubo, sizeof(UniformStructure));
}

void OMTestRenderer::keyInput(bool w, bool a, bool s, bool d, bool lsh, bool sp, bool upk, bool downk, bool leftk,
                              bool rightk)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float time = std::chrono::duration<float>(currentTime - startTime).count();
    startTime = currentTime;

    glm::vec3 front;
    front.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    front.y = std::sin(glm::radians(m_pitch));
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

    if (upk)
    {
        m_pitch += m_cameraRotateSpeed * time;
    }
    if (downk)
    {
        m_pitch -= m_cameraRotateSpeed * time;
    }
    if (leftk)
    {
        m_yaw -= m_cameraRotateSpeed * time;
    }
    if (rightk)
    {
        m_yaw += m_cameraRotateSpeed * time;
    }

    if (m_yaw < 0.0f)
        m_yaw += 360.0f;
    m_yaw = std::fmod(m_yaw + 180.0f, 360.0f);
    m_yaw -= 180.0f;

    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    if (w || a || s || d || lsh || sp || upk || downk || leftk || rightk)
    {
        logger.info("pitch: {} yaw: {} {} {} {}", m_pitch, m_yaw, m_cameraPos.x, m_cameraPos.y, m_cameraPos.z);
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
        commandBuffer.bindVertexBuffers(0, std::vector{vertexBuffer}, std::vector<DeviceSize>{0});
        commandBuffer.bindIndexBuffer(indexBuffer, 0, IndexType::eUint32);
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
    renderer->logicalDevice.destroyImageView(textureImageView, renderer->allocator);
    renderer->logicalDevice.freeMemory(stagingBufferMemory, renderer->allocator);
    renderer->logicalDevice.destroyBuffer(stagingBuffer, renderer->allocator);
    renderer->logicalDevice.freeMemory(imageMemory, renderer->allocator);
    renderer->logicalDevice.destroyImage(textureImage, renderer->allocator);
    renderer->logicalDevice.freeDescriptorSets(descriptorPool, descriptorSet);
    renderer->logicalDevice.destroyDescriptorPool(descriptorPool, renderer->allocator);
    renderer->logicalDevice.unmapMemory(uniformBufferMemory);
    renderer->logicalDevice.freeMemory(uniformBufferMemory, renderer->allocator);
    renderer->logicalDevice.destroyBuffer(uniformBuffer, renderer->allocator);
    for (auto l : descriptorSetLayouts)
    {
        renderer->logicalDevice.destroyDescriptorSetLayout(l, renderer->allocator);
    }
    renderer->logicalDevice.destroyBuffer(indexBuffer, renderer->allocator);
    renderer->logicalDevice.destroyBuffer(vertexBuffer, renderer->allocator);
    renderer->logicalDevice.freeMemory(indexBufferMemory, renderer->allocator);
    renderer->logicalDevice.freeMemory(vertexBufferMemory, renderer->allocator);
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
