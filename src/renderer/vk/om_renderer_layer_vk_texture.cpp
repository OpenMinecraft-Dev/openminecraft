#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"

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

OMRendererTextureVk::OMRendererTextureVk(uint64_t width, uint64_t height, common::OMTextureType type, common::OMTextureArrangement arr, OMRendererVk *renderer): OMRendererTexture(width, height, type, arr, renderer)
{
    auto memprop = renderer->physicalDevice.getMemoryProperties();
    image = renderer->logicalDevice.createImage(
            ImageCreateInfo({}, fromCommonType(type), fromCommonUsage(arr), Extent3D(width, height, 1), 1, 1,
                            SampleCountFlagBits::e1, ImageTiling::eOptimal,
                            ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled, SharingMode::eExclusive,
                            {}, ImageLayout::eUndefined),
            renderer->allocator);

    auto req = renderer->logicalDevice.getImageMemoryRequirements(image);
    imageMemory = renderer->logicalDevice.allocateMemory(
        MemoryAllocateInfo(req.size,
                           findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eDeviceLocal, memprop)),
        renderer->allocator);

    renderer->logicalDevice.bindImageMemory(image, imageMemory, 0);

    // TODO: copy data!!

    imageView = renderer->logicalDevice.createImageView(
            ImageViewCreateInfo({}, image, ImageViewType::e2D, Format::eR8G8B8A8Srgb, {},
                                ImageSubresourceRange(ImageAspectFlagBits::eColor, 0, 1, 0, 1)),
            renderer->allocator);
}

OMRendererTextureVk::~OMRendererTextureVk()
{

}
}