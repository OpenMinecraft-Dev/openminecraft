#include "openminecraft/renderer/vk/om_renderer_layer_vk_testrenderer.hpp"

#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

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

OMTestRenderer::OMTestRenderer(OMRendererVk *renderer) : renderer(renderer)
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

    auto attachments = std::vector{
        AttachmentDescription({}, renderer->swapchainManager->format.format, SampleCountFlagBits::e1,
                              AttachmentLoadOp::eClear, AttachmentStoreOp::eStore, AttachmentLoadOp::eDontCare,
                              AttachmentStoreOp::eDontCare, ImageLayout::eUndefined, ImageLayout::ePresentSrcKHR)};
    auto subpasses = std::vector{SubpassDescription({}, PipelineBindPoint::eGraphics, nullptr, attaches, nullptr)};
    auto depe =
        std::vector{SubpassDependency(VK_SUBPASS_EXTERNAL, 0, PipelineStageFlagBits::eColorAttachmentOutput,
                                      PipelineStageFlagBits::eColorAttachmentOutput, {},
                                      AccessFlagBits::eColorAttachmentRead | AccessFlagBits::eColorAttachmentWrite)};

    renderPass = renderer->logicalDevice.createRenderPass(RenderPassCreateInfo({}, attachments, subpasses, depe),
                                                          renderer->allocator);

    auto prop = renderer->physicalDevice.getMemoryProperties();
    {
        auto siz = (3 + 2 + 2) * 4 * sizeof(float);
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

        float arr[] = {-0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f,  -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                       0.5f,  0.5f,  0.0f, 0.0f, 1.0f, 0.0f, 1.0f,
            -0.5f, 0.5f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f};

        std::memcpy(r, arr, siz);

        renderer->logicalDevice.unmapMemory(vertexBufferMemory);
    }

    {
        auto siz = 6 * sizeof(uint32_t);
        indexBuffer = renderer->logicalDevice.createBuffer(
            BufferCreateInfo({}, siz, BufferUsageFlagBits::eIndexBuffer, SharingMode::eExclusive), renderer->allocator);

        auto req = renderer->logicalDevice.getBufferMemoryRequirements(indexBuffer);
        indexBufferMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(
                req.size,
                findMemoryType(req.memoryTypeBits,
                               MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent, prop)),
            renderer->allocator);

        renderer->logicalDevice.bindBufferMemory(indexBuffer, indexBufferMemory, 0);

        auto r = renderer->logicalDevice.mapMemory(indexBufferMemory, 0, siz);

        uint32_t arr[] = {0, 1, 2, 2, 3, 0};

        std::memcpy(r, arr, sizeof(uint32_t) * 6);

        renderer->logicalDevice.unmapMemory(indexBufferMemory);
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
    descriptorSetLayouts.emplace_back(renderer->logicalDevice.createDescriptorSetLayout(DescriptorSetLayoutCreateInfo({}, b), renderer->allocator));

    const std::vector b2 = {
        DescriptorSetLayoutBinding(0, DescriptorType::eCombinedImageSampler, 1, ShaderStageFlagBits::eFragment)};
    descriptorSetLayouts.emplace_back(renderer->logicalDevice.createDescriptorSetLayout(DescriptorSetLayoutCreateInfo({}, b2), renderer->allocator));

    const std::vector a = {DescriptorPoolSize(DescriptorType::eUniformBuffer, renderer->framesInFlight), DescriptorPoolSize(DescriptorType::eCombinedImageSampler, 1)};

    descriptorPool = renderer->logicalDevice.createDescriptorPool(
        DescriptorPoolCreateInfo(DescriptorPoolCreateFlagBits::eFreeDescriptorSet, renderer->framesInFlight + 1, a),
        renderer->allocator);
    descriptorSet = renderer->logicalDevice.allocateDescriptorSets(DescriptorSetAllocateInfo(descriptorPool, descriptorSetLayouts[0]))[0];

    const std::vector c = {DescriptorBufferInfo(uniformBuffer, 0, sizeof(UniformStructure))};
    renderer->logicalDevice.updateDescriptorSets(
    WriteDescriptorSet(descriptorSet, 0, 0, DescriptorType::eUniformBuffer, {}, c), nullptr);

    {
        int texWidth, texHeight, texChannels;
        stbi_uc *pixels = stbi_load("/usr/share/wallpapers/Next/contents/images_dark/5120x2880.png", &texWidth, &texHeight,
                                    &texChannels, STBI_rgb_alpha);
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
            ImageCreateInfo({}, ImageType::e2D, Format::eR8G8B8A8Srgb, Extent3D(texWidth, texHeight, 1), 1, 1, SampleCountFlagBits::e1,
                            ImageTiling::eOptimal, ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled,
                            SharingMode::eExclusive, {}, ImageLayout::eUndefined),
            renderer->allocator);

        req = renderer->logicalDevice.getImageMemoryRequirements(textureImage);
        imageMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(req.size,
                               findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eDeviceLocal, prop)),
            renderer->allocator);

        renderer->logicalDevice.bindImageMemory(textureImage, imageMemory, 0);

        transitionImageLayout(textureImage, Format::eR8G8B8A8Srgb, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal);
        copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
        transitionImageLayout(textureImage, Format::eR8G8B8A8Srgb, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal);

        textureImageView = renderer->logicalDevice.createImageView(ImageViewCreateInfo({}, textureImage, ImageViewType::e2D, Format::eR8G8B8A8Srgb, {}, ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1)), renderer->allocator);
    }

    {
        auto prop = renderer->physicalDevice.getProperties();
        auto fea = renderer->physicalDevice.getFeatures();

        textureSampler = renderer->logicalDevice.createSampler(SamplerCreateInfo({}, Filter::eLinear, Filter::eLinear, SamplerMipmapMode::eLinear, SamplerAddressMode::eRepeat, SamplerAddressMode::eRepeat, SamplerAddressMode::eRepeat, 0.0f, fea.samplerAnisotropy, prop.limits.maxSamplerAnisotropy, false, CompareOp::eAlways, 0.0f, 0.0f, BorderColor::eIntOpaqueBlack, false), renderer->allocator);
    }

    combinedDescriptorSet = renderer->logicalDevice.allocateDescriptorSets(DescriptorSetAllocateInfo(descriptorPool, descriptorSetLayouts[1]))[0];

    const auto cc = DescriptorImageInfo(textureSampler, textureImageView, ImageLayout::eShaderReadOnlyOptimal);
    renderer->logicalDevice.updateDescriptorSets(WriteDescriptorSet(combinedDescriptorSet, 0, 0, DescriptorType::eCombinedImageSampler, cc), nullptr);

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
    cmd.copyBufferToImage(buffer, image, ImageLayout::eTransferDstOptimal, BufferImageCopy(0, 0, 0, ImageSubresourceLayers(ImageAspectFlagBits::eColor, 0, 0, 1), Offset3D(0, 0, 0), Extent3D(width, height, 1)));
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
    static auto startTime = std::chrono::high_resolution_clock::now();
    const auto currentTime = std::chrono::high_resolution_clock::now();
    const float timee = std::chrono::duration<float>(currentTime - startTime).count();
    UniformStructure ubo{};
    ubo.model = glm::rotate(glm::mat4(1.0f), timee * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f),
                                static_cast<float>(renderer->swapchainManager->extent.width) /
                                    static_cast<float>(renderer->swapchainManager->extent.height),
                                0.1f, 20.0f);
    ubo.proj[1][1] *= -1;

    std::memcpy(mappedUniformBuffer, &ubo, sizeof(UniformStructure));
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
        renderer->logicalDevice.destroyPipeline(pipeline, renderer->allocator);
        renderer->logicalDevice.destroyPipelineLayout(pipelineLayout, renderer->allocator);
    }

    pipelineLayout = renderer->logicalDevice.createPipelineLayout(PipelineLayoutCreateInfo({}, descriptorSetLayouts), renderer->allocator);

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

        const std::vector bi = {VertexInputBindingDescription(0, (2 + 3 + 2) * sizeof(float), VertexInputRate::eVertex)};
        const std::vector ad = {VertexInputAttributeDescription(0, 0, Format::eR32G32Sfloat, 0),
                                VertexInputAttributeDescription(1, 0, Format::eR32G32B32Sfloat, 2 * sizeof(float)),
                                VertexInputAttributeDescription(2, 0, Format::eR32G32Sfloat, 5 * sizeof(float))};

        auto vertexInput = PipelineVertexInputStateCreateInfo({}, bi, ad);
        auto inputAssembly = PipelineInputAssemblyStateCreateInfo({}, PrimitiveTopology::eTriangleList, false);

        const std::vector vp = {Viewport(0, 0, static_cast<float>(renderer->swapchainManager->extent.width),
                                         static_cast<float>(renderer->swapchainManager->extent.height), 0, 1)};
        const std::vector scis = {Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent)};
        auto viewportState = PipelineViewportStateCreateInfo({}, vp, scis);
        auto rasterization =
            PipelineRasterizationStateCreateInfo({}, false, false, PolygonMode::eFill, CullModeFlagBits::eBack,
                                                 FrontFace::eCounterClockwise, true, 0, 0, 0, 1);
        auto multisample = PipelineMultisampleStateCreateInfo({}, SampleCountFlagBits::e1, false);

        const std::vector attc = {
            PipelineColorBlendAttachmentState(false, {}, {}, {}, {}, {}, {},
                                              ColorComponentFlagBits::eA | ColorComponentFlagBits::eR |
                                                  ColorComponentFlagBits::eG | ColorComponentFlagBits::eB)};
        auto colorblend =
            PipelineColorBlendStateCreateInfo({}, true, LogicOp::eCopy, attc, std::array{0.f, 0.f, 0.f, 0.f});

        auto result = renderer->logicalDevice.createGraphicsPipeline(
            {},
            GraphicsPipelineCreateInfo({}, shaders, &vertexInput, &inputAssembly, {}, &viewportState, &rasterization,
                                       &multisample, {}, &colorblend, {}, pipelineLayout, renderPass, 0, {}, -1),
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

    for (auto img : renderer->swapchainManager->swapchainImageViews)
    {
        framebuffers.push_back(renderer->logicalDevice.createFramebuffer(
            FramebufferCreateInfo({}, renderPass, img, renderer->swapchainManager->extent.width,
                                  renderer->swapchainManager->extent.height, 1),
            renderer->allocator));
    }

    int i = 0;
    for (auto framebuffer : framebuffers)
    {
        auto commandBuffer = renderer->logicalDevice.allocateCommandBuffers(
            CommandBufferAllocateInfo(commandPool, CommandBufferLevel::ePrimary, 1))[0];

        commandBuffer.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eSimultaneousUse));
        auto test = std::vector<ClearValue>();
        for (int i = 0; i < renderer->swapchainManager->swapchainImageViews.size(); i++)
        {
            test.push_back(ClearValue({55, 55, 55, 55}));
        }
        commandBuffer.beginRenderPass(RenderPassBeginInfo(renderPass, framebuffer,
                                                          Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent),
                                                          test),
                                      SubpassContents::eInline);
        commandBuffer.bindPipeline(PipelineBindPoint::eGraphics, pipeline);
        commandBuffer.bindDescriptorSets(PipelineBindPoint::eGraphics, pipelineLayout, 0, std::vector{descriptorSet, combinedDescriptorSet}, nullptr);
        commandBuffer.bindVertexBuffers(0, std::vector{vertexBuffer}, std::vector<DeviceSize>{0});
        commandBuffer.bindIndexBuffer(indexBuffer, 0, IndexType::eUint32);
        commandBuffer.drawIndexed(6, 1, 0, 0, 0);
        commandBuffer.endRenderPass();
        commandBuffer.end();

        commandBuffers.push_back(commandBuffer);
        i++;
    }
}
void OMTestRenderer::destroy()
{
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