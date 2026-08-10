#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"

#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

using namespace ::vk;
using namespace openminecraft::i18n::res;

namespace openminecraft::renderer::vk
{
static auto findMemoryType(uint32_t typeFilter, MemoryPropertyFlags properties,
                           const PhysicalDeviceMemoryProperties &memProperties) -> uint32_t
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

static auto fromCommonType(common::OMTextureType type) -> ImageType
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

static auto fromCommonType2(common::OMTextureType type) -> ImageViewType
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

static auto fromCommonUsage(common::OMTextureArrangement arr) -> Format
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

static auto fromCommonType(common::OMTextureAddressMode a) -> SamplerAddressMode
{
    switch (a)
    {
    case common::Repeat:
        return SamplerAddressMode::eRepeat;
    default:
    case common::ClampToEdge:
        return SamplerAddressMode::eClampToEdge;
    case common::ClampToBorder:
        return SamplerAddressMode::eClampToBorder;
    }
}

static auto fromCommonType(common::OMTextureBorder b) -> BorderColor
{
    switch (b)
    {
    default:
    case common::OpaqueBlack:
        return BorderColor::eFloatOpaqueBlack;
    case common::OpaqueWhite:
        return BorderColor::eFloatOpaqueWhite;
    case common::TransparentBlack:
        return BorderColor::eFloatTransparentBlack;
    }
}

OMRendererTextureVk::OMRendererTextureVk(uint64_t width, uint64_t height, uint64_t mipmap, common::OMTextureType type,
                                         common::OMTextureArrangement arr, OMRendererVk *renderer)
    : OMRendererTexture(width, height, mipmap, type, arr, reinterpret_cast<OMRenderer *>(renderer)), renderer(renderer),
      mipmap(mipmap)
{
    try
    {
        auto memprop = renderer->physicalDevice.getMemoryProperties();
        format = fromCommonUsage(arr);
        image = renderer->logicalDevice.createImage(
            ImageCreateInfo({}, fromCommonType(type), format, Extent3D(width, height, 1), mipmap + 1, 1,
                            SampleCountFlagBits::e1, ImageTiling::eOptimal,
                            (arr == common::Depth)
                                ? ImageUsageFlagBits::eDepthStencilAttachment
                                : ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled |
                                      ImageUsageFlagBits::eColorAttachment | ImageUsageFlagBits::eTransferSrc,
                            SharingMode::eExclusive, {}, ImageLayout::eUndefined),
            renderer->allocator);

        auto req = renderer->logicalDevice.getImageMemoryRequirements(image);
        imageMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(req.size,
                               findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eDeviceLocal, memprop)),
            renderer->allocator);

        renderer->logicalDevice.bindImageMemory(image, imageMemory, 0);

        imageView = renderer->logicalDevice.createImageView(
            ImageViewCreateInfo({}, image, fromCommonType2(type), format, {},
                                ImageSubresourceRange(((arr == common::Depth) ? ImageAspectFlagBits::eDepth
                                                                              : ImageAspectFlagBits::eColor),
                                                      0, mipmap + 1, 0, 1)),
            renderer->allocator);
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.create"));
    }
}

static auto toFilter(common::OMTextureFilter f) -> Filter
{
    switch (f)
    {
    default:
    case common::Linear:
        return Filter::eLinear;
    case common::Nearest:
        return Filter::eNearest;
    }
}

static auto toMipFilter(common::OMTextureFilter f) -> SamplerMipmapMode
{
    switch (f)
    {
    default:
    case common::Linear:
        return SamplerMipmapMode::eLinear;
    case common::Nearest:
        return SamplerMipmapMode::eNearest;
    }
}

void OMRendererTextureVk::setupSampler()
{
    try
    {
        auto prop = renderer->physicalDevice.getProperties();
        auto fea = renderer->physicalDevice.getFeatures();

        sampler = renderer->logicalDevice.createSampler(
            SamplerCreateInfo({}, toFilter(magFilter), toFilter(minFilter), toMipFilter(mipFilter),
                              fromCommonType(addressModeU), fromCommonType(addressModeV),
                              SamplerAddressMode::eClampToEdge, 0.0f, fea.samplerAnisotropy,
                              prop.limits.maxSamplerAnisotropy, false, CompareOp::eAlways, 0.0f, mipmap + 1,
                              fromCommonType(border), false),
            renderer->allocator);

        if (mipmap > 0)
        {
            auto cmdBuff = renderer->logicalDevice.allocateCommandBuffers(
                CommandBufferAllocateInfo(renderer->tempCommandPool, CommandBufferLevel::ePrimary, 1))[0];
            cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));

            transitionImageLayout(cmdBuff, ImageLayout::eShaderReadOnlyOptimal, ImageLayout::eTransferSrcOptimal, 0);

            auto w = width;
            auto h = height;
            for (int i = 1; i <= mipmap; ++i)
            {
                auto cw = std::max(1, static_cast<int>(w >> 1));
                auto ch = std::max(1, static_cast<int>(h >> 1));

                transitionImageLayout(cmdBuff, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal, i);
                auto blit = ImageBlit(ImageSubresourceLayers(ImageAspectFlagBits::eColor, i - 1, 0, 1),
                                      {Offset3D(0, 0, 0), Offset3D(w, h, 1)},
                                      ImageSubresourceLayers(ImageAspectFlagBits::eColor, i, 0, 1),
                                      {Offset3D(0, 0, 0), Offset3D(cw, ch, 1)});
                cmdBuff.blitImage(image, ImageLayout::eTransferSrcOptimal, image, ImageLayout::eTransferDstOptimal, 1,
                                  &blit, Filter::eLinear);
                transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eTransferSrcOptimal, i);
                transitionImageLayout(cmdBuff, ImageLayout::eTransferSrcOptimal, ImageLayout::eShaderReadOnlyOptimal,
                                      i - 1);

                w = cw;
                h = ch;
            }
            transitionImageLayout(cmdBuff, ImageLayout::eTransferSrcOptimal, ImageLayout::eShaderReadOnlyOptimal,
                                  mipmap);

            cmdBuff.end();
            renderer->queues.first.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff));
            renderer->queues.first.waitIdle();

            renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPool, 1, &cmdBuff);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.create"));
    }
}

OMRendererTextureVk::~OMRendererTextureVk()
{
    try
    {
        renderer->logicalDevice.destroySampler(sampler, renderer->allocator);
        renderer->logicalDevice.destroyImageView(imageView, renderer->allocator);
        renderer->logicalDevice.freeMemory(imageMemory, renderer->allocator);
        renderer->logicalDevice.destroyImage(image, renderer->allocator);
    }
    catch (SystemError &e)
    {
    }
}

static auto toAccessMask(ImageLayout layout) -> AccessFlagBits
{
    switch (layout)
    {
    case ImageLayout::eTransferDstOptimal:
        return AccessFlagBits::eTransferWrite;
    case ImageLayout::eTransferSrcOptimal:
        return AccessFlagBits::eTransferRead;
    case ImageLayout::eShaderReadOnlyOptimal:
        return AccessFlagBits::eShaderRead;
    case ImageLayout::eUndefined:
    default:
        return {};
    }
}

static auto toStage(ImageLayout layout) -> PipelineStageFlagBits
{
    switch (layout)
    {
    case ImageLayout::eTransferSrcOptimal:
    case ImageLayout::eTransferDstOptimal:
        return PipelineStageFlagBits::eTransfer;
    case ImageLayout::eShaderReadOnlyOptimal:
        return PipelineStageFlagBits::eFragmentShader;
    case ImageLayout::eUndefined:
    default:
        return PipelineStageFlagBits::eTopOfPipe;
    }
}

void OMRendererTextureVk::transitionImageLayout(CommandBuffer cmd, ImageLayout oldLayout, ImageLayout newLayout,
                                                uint64_t level)
{
    auto barrier = ImageMemoryBarrier({}, {}, oldLayout, newLayout, QueueFamilyIgnored, QueueFamilyIgnored, image,
                                      ImageSubresourceRange(ImageAspectFlagBits::eColor, level, 1, 0, 1));

    PipelineStageFlagBits sourceStage = toStage(oldLayout), destinationStage = toStage(newLayout);

    barrier.srcAccessMask = toAccessMask(oldLayout);
    barrier.dstAccessMask = toAccessMask(newLayout);

    cmd.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);
}

void OMRendererTextureVk::updateData(void *p)
{
    try
    {
        auto stagBuffer =
            renderer->allocateBuffer(common::Misc, width * height * ((this->arr == common::ColorRgb) ? 3 : 4));
        stagBuffer->updateData(p);

        auto cmdBuff = renderer->logicalDevice.allocateCommandBuffers(
            CommandBufferAllocateInfo(renderer->tempCommandPool, CommandBufferLevel::ePrimary, 1))[0];

        cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));
        transitionImageLayout(cmdBuff, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal, 0);
        cmdBuff.copyBufferToImage(
            reinterpret_cast<OMRendererBufferVk *>(stagBuffer)->buffer, image, ImageLayout::eTransferDstOptimal,
            BufferImageCopy(0, width, height,
                            ImageSubresourceLayers((this->arr == common::Depth) ? ImageAspectFlagBits::eDepth
                                                                                : ImageAspectFlagBits::eColor,
                                                   0, 0, 1),
                            Offset3D(0, 0, 0), Extent3D(width, height, 1)));
        transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal, 0);

        cmdBuff.end();
        renderer->queues.first.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff));
        renderer->queues.first.waitIdle();

        renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPool, 1, &cmdBuff);

        delete stagBuffer;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.update"));
    }
}

void OMRendererTextureVk::updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h)
{
    try
    {
        auto stagBuffer = renderer->allocateBuffer(common::Misc, w * h * ((this->arr == common::ColorRgb) ? 3 : 4));
        stagBuffer->updateData(p);

        auto cmdBuff = renderer->logicalDevice.allocateCommandBuffers(
            CommandBufferAllocateInfo(renderer->tempCommandPool, CommandBufferLevel::ePrimary, 1))[0];

        cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));
        transitionImageLayout(cmdBuff, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal, 0);
        cmdBuff.copyBufferToImage(
            reinterpret_cast<OMRendererBufferVk *>(stagBuffer)->buffer, image, ImageLayout::eTransferDstOptimal,
            BufferImageCopy(0, w, h,
                            ImageSubresourceLayers((this->arr == common::Depth) ? ImageAspectFlagBits::eDepth
                                                                                : ImageAspectFlagBits::eColor,
                                                   0, 0, 1),
                            Offset3D(x, y, 0), Extent3D(w, h, 1)));
        transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal, 0);

        cmdBuff.end();
        renderer->queues.first.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff));
        renderer->queues.first.waitIdle();

        renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPool, 1, &cmdBuff);

        delete stagBuffer;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.update"));
    }
}
} // namespace openminecraft::renderer::vk
