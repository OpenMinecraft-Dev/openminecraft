#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"

#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include <iostream>

using namespace ::vk;

namespace openminecraft::renderer::vk
{
static uint32_t findMemoryType(uint32_t typeFilter, MemoryPropertyFlags properties,
                               const PhysicalDeviceMemoryProperties &memProperties)
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

static ImageType fromCommonType(common::OMTextureType type)
{
    switch (type)
    {
    case common::Dim1:
        return ImageType::e1D;
    default:
    case common::Dim2:
        return ImageType::e2D;
    case common::Dim3:
        return ImageType::e3D;
    }
}

static ImageViewType fromCommonType2(common::OMTextureType type)
{
    switch (type)
    {
    case common::Dim1:
        return ImageViewType::e1D;
    default:
    case common::Dim2:
        return ImageViewType::e2D;
    case common::Dim3:
        return ImageViewType::e3D;
    }
}

static Format fromCommonUsage(common::OMTextureArrangement arr)
{
    switch (arr)
    {
    case common::Depth:
        return Format::eD32Sfloat;
    case common::ColorRgb:
        return Format::eR8G8B8Srgb;
    default:
    case common::ColorRgba:
        return Format::eR8G8B8A8Srgb;
    }
}

OMRendererTextureVk::OMRendererTextureVk(uint64_t width, uint64_t height, common::OMTextureType type,
                                         common::OMTextureArrangement arr, OMRendererVk *renderer)
    : OMRendererTexture(width, height, type, arr, reinterpret_cast<OMRenderer *>(renderer)), renderer(renderer)
{
    auto memprop = renderer->physicalDevice.getMemoryProperties();
    format = fromCommonUsage(arr);
    image = renderer->logicalDevice.createImage(
        ImageCreateInfo({}, fromCommonType(type), format, Extent3D(width, height, 1), 1, 1, SampleCountFlagBits::e1,
                        ImageTiling::eOptimal,
                        (arr == common::Depth) ? ImageUsageFlagBits::eDepthStencilAttachment
                                               : ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled,
                        SharingMode::eExclusive, {}, ImageLayout::eUndefined),
        renderer->allocator);

    auto req = renderer->logicalDevice.getImageMemoryRequirements(image);
    imageMemory = renderer->logicalDevice.allocateMemory(
        MemoryAllocateInfo(req.size, findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eDeviceLocal, memprop)),
        renderer->allocator);

    renderer->logicalDevice.bindImageMemory(image, imageMemory, 0);

    imageView = renderer->logicalDevice.createImageView(
        ImageViewCreateInfo({}, image, fromCommonType2(type), format, {},
                            ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1)),
        renderer->allocator);

    // TODO: copy data!!
}

OMRendererTextureVk::~OMRendererTextureVk()
{
    renderer->logicalDevice.destroyImageView(imageView, renderer->allocator);
    renderer->logicalDevice.freeMemory(imageMemory, renderer->allocator);
    renderer->logicalDevice.destroyImage(image, renderer->allocator);
}

void OMRendererTextureVk::transitionImageLayout(CommandBuffer cmd, ImageLayout oldLayout, ImageLayout newLayout)
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

    cmd.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);
}

void OMRendererTextureVk::updateData(void *p)
{
    auto stagBuffer =
        renderer->allocateBuffer(common::Misc, width * height * ((this->arr == common::ColorRgb) ? 3 : 4));
    stagBuffer->updateData(p);

    auto cmdBuff = renderer->logicalDevice.allocateCommandBuffers(
        CommandBufferAllocateInfo(renderer->tempCommandPool, CommandBufferLevel::ePrimary, 1))[0];

    cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));
    transitionImageLayout(cmdBuff, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal);
    cmdBuff.copyBufferToImage(
        reinterpret_cast<OMRendererBufferVk *>(stagBuffer)->buffer, image, ImageLayout::eTransferDstOptimal,
        BufferImageCopy(0, 0, 0,
                        ImageSubresourceLayers((this->arr == common::Depth) ? ImageAspectFlagBits::eDepth
                                                                            : ImageAspectFlagBits::eColor,
                                               0, 0, 1),
                        Offset3D(0, 0, 0), Extent3D(width, height, 1)));
    transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal);

    cmdBuff.end();
    renderer->queues.first.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff));
    renderer->queues.first.waitIdle();

    renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPool, 1, &cmdBuff);

    delete stagBuffer;

    /*imageView = renderer->logicalDevice.createImageView(
        ImageViewCreateInfo({}, image, ImageViewType::e2D, Format::eR8G8B8A8Srgb, {},
                            ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1)),
        renderer->allocator);*/
}
} // namespace openminecraft::renderer::vk
