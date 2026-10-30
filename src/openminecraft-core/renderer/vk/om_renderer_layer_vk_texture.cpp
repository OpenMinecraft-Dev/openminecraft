#include "openminecraft/renderer/vk/om_renderer_layer_vk_texture.hpp"

#include "openminecraft/i18n/om_i18n_res.hpp"
#include "openminecraft/log/om_log_common.hpp"
#include "openminecraft/renderer/common/om_renderer_buffer.hpp"
#include "openminecraft/renderer/common/om_renderer_texture.hpp"
#include "openminecraft/renderer/om_renderer_exception.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_buffer.hpp"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cstdint>

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

static auto fromCommonType2(common::OMTextureType type) -> ImageViewType
{
    switch (type)
    {
    default:
    case common::Dim2:
    case common::Dim2Multisample:
        return ImageViewType::e2D;
    case common::Dim2Array:
        return ImageViewType::e2DArray;
    }
}

static auto fromCommonUsage(common::OMTextureArrangement arr) -> Format
{
    switch (arr)
    {
    case common::D32Sfloat:
        return Format::eD32Sfloat;
    case common::D32SfloatS8Uint:
        return Format::eD32SfloatS8Uint;
    case common::D24SfloatS8Uint:
        return Format::eD24UnormS8Uint;
    case common::R8G8B8Srgb:
        return Format::eR8G8B8Srgb;
    default:
    case common::R8G8B8A8Srgb:
        return Format::eR8G8B8A8Srgb;
    case common::R8Sint:
        return Format::eR8Sint;
    case common::R8G8Sint:
        return Format::eR8G8Sint;
    case common::R8G8B8Sint:
        return Format::eR8G8B8Sint;
    case common::R8G8B8A8Sint:
        return Format::eR8G8B8A8Sint;
    case common::R8Uint:
        return Format::eR8Uint;
    case common::R8G8Uint:
        return Format::eR8G8Uint;
    case common::R8G8B8Uint:
        return Format::eR8G8B8Uint;
    case common::R8G8B8A8Uint:
        return Format::eR8G8B8A8Uint;
    case common::R8Snorm:
        return Format::eR8Snorm;
    case common::R8G8Snorm:
        return Format::eR8G8Snorm;
    case common::R8G8B8Snorm:
        return Format::eR8G8B8Snorm;
    case common::R8G8B8A8Snorm:
        return Format::eR8G8B8A8Snorm;
    case common::R8Unorm:
        return Format::eR8Unorm;
    case common::R8G8Unorm:
        return Format::eR8G8Unorm;
    case common::R8G8B8Unorm:
        return Format::eR8G8B8Unorm;
    case common::R8G8B8A8Unorm:
        return Format::eR8G8B8A8Unorm;
    case common::R16Sfloat:
        return Format::eR16Sfloat;
    case common::R16G16Sfloat:
        return Format::eR16G16Sfloat;
    case common::R16G16B16Sfloat:
        return Format::eR16G16B16Sfloat;
    case common::R16G16B16A16Sfloat:
        return Format::eR16G16B16A16Sfloat;
    case common::R16Sint:
        return Format::eR16Sint;
    case common::R16G16Sint:
        return Format::eR16G16Sint;
    case common::R16G16B16Sint:
        return Format::eR16G16B16Sint;
    case common::R16G16B16A16Sint:
        return Format::eR16G16B16A16Sint;
    case common::R16Uint:
        return Format::eR16Uint;
    case common::R16G16Uint:
        return Format::eR16G16Uint;
    case common::R16G16B16Uint:
        return Format::eR16G16B16Uint;
    case common::R16G16B16A16Uint:
        return Format::eR16G16B16A16Uint;
    case common::R16Snorm:
        return Format::eR16Snorm;
    case common::R16G16Snorm:
        return Format::eR16G16Snorm;
    case common::R16G16B16Snorm:
        return Format::eR16G16B16Snorm;
    case common::R16G16B16A16Snorm:
        return Format::eR16G16B16A16Snorm;
    case common::R16Unorm:
        return Format::eR16Unorm;
    case common::R16G16Unorm:
        return Format::eR16G16Unorm;
    case common::R16G16B16Unorm:
        return Format::eR16G16B16Unorm;
    case common::R16G16B16A16Unorm:
        return Format::eR16G16B16A16Unorm;
    case common::R32Sfloat:
        return Format::eR32Sfloat;
    case common::R32G32Sfloat:
        return Format::eR32G32Sfloat;
    case common::R32G32B32Sfloat:
        return Format::eR32G32B32Sfloat;
    case common::R32G32B32A32Sfloat:
        return Format::eR32G32B32A32Sfloat;
    case common::R32Sint:
        return Format::eR32Sint;
    case common::R32G32Sint:
        return Format::eR32G32Sint;
    case common::R32G32B32Sint:
        return Format::eR32G32B32Sint;
    case common::R32G32B32A32Sint:
        return Format::eR32G32B32A32Sint;
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

OMRendererTextureVk::OMRendererTextureVk(uint64_t width, uint64_t height, uint64_t layers, uint64_t mipmap,
                                         common::OMTextureType type, common::OMTextureArrangement arr,
                                         OMRendererVk *renderer)
    : OMRendererTexture(width, height, layers, mipmap, type, arr, reinterpret_cast<OMRenderer *>(renderer)),
      mipmap(mipmap), layers(layers), renderer(renderer)
{
    if (type == common::Dim2Multisample)
    {
#define DET(n)                                                                                                         \
    if (layers <= n)                                                                                                   \
    {                                                                                                                  \
        sampleCount = SampleCountFlagBits::e##n;                                                                       \
    }
        DET(1)
        else DET(2) else DET(4) else DET(8) else DET(16) else DET(32) else
        {
            sampleCount = SampleCountFlagBits::e64;
        }
        layers = 1;
    }
    else
    {
        sampleCount = SampleCountFlagBits::e1;
    }

    try
    {
        auto memprop = renderer->physicalDevice.getMemoryProperties();
        format = fromCommonUsage(arr);
        std::vector<uint32_t> fams;
        fams.push_back(renderer->transferQueueIndex);
        fams.push_back(renderer->queueFamilyIndex.first);
        image = renderer->logicalDevice.createImage(
            ImageCreateInfo({}, ImageType::e2D, format, Extent3D(width, height, 1), mipmap + 1, layers, sampleCount,
                            ImageTiling::eOptimal,
                            (!isColorFormat(arr))
                                ? ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled |
                                      ImageUsageFlagBits::eDepthStencilAttachment | ImageUsageFlagBits::eTransferSrc
                                : ImageUsageFlagBits::eTransferDst | ImageUsageFlagBits::eSampled |
                                      ImageUsageFlagBits::eColorAttachment | ImageUsageFlagBits::eTransferSrc,
                            SharingMode::eConcurrent, fams, ImageLayout::eUndefined),
            renderer->allocator);

        auto req = renderer->logicalDevice.getImageMemoryRequirements(image);
        imageMemory = renderer->logicalDevice.allocateMemory(
            MemoryAllocateInfo(req.size,
                               findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eDeviceLocal, memprop)),
            renderer->allocator);

        renderer->logicalDevice.bindImageMemory(image, imageMemory, 0);

        imageView = renderer->logicalDevice.createImageView(
            ImageViewCreateInfo({}, image, fromCommonType2(type), format, {},
                                ImageSubresourceRange(
                                    ((!isColorFormat(arr)) ? ImageAspectFlagBits::eDepth : ImageAspectFlagBits::eColor),
                                    0, mipmap + 1, 0, layers)),
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
                CommandBufferAllocateInfo(renderer->tempCommandPoolTransfer, CommandBufferLevel::ePrimary, 1))[0];
            cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));

            transitionImageLayout(cmdBuff, ImageLayout::eShaderReadOnlyOptimal, ImageLayout::eTransferSrcOptimal, 0, 0,
                                  layers);

            auto w = width;
            auto h = height;
            for (int i = 1; i <= mipmap; ++i)
            {
                auto cw = std::max(1, static_cast<int>(w >> 1));
                auto ch = std::max(1, static_cast<int>(h >> 1));

                transitionImageLayout(cmdBuff, ImageLayout::eUndefined, ImageLayout::eTransferDstOptimal, i, 0, layers);
                auto blit = ImageBlit(ImageSubresourceLayers(ImageAspectFlagBits::eColor, i - 1, 0, layers),
                                      {Offset3D(0, 0, 0), Offset3D(w, h, 1)},
                                      ImageSubresourceLayers(ImageAspectFlagBits::eColor, i, 0, layers),
                                      {Offset3D(0, 0, 0), Offset3D(cw, ch, 1)});
                cmdBuff.blitImage(image, ImageLayout::eTransferSrcOptimal, image, ImageLayout::eTransferDstOptimal, 1,
                                  &blit, Filter::eLinear);
                transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eTransferSrcOptimal, i, 0,
                                      layers);
                transitionImageLayout(cmdBuff, ImageLayout::eTransferSrcOptimal, ImageLayout::eShaderReadOnlyOptimal,
                                      i - 1, 0, layers);

                w = cw;
                h = ch;
            }
            transitionImageLayout(cmdBuff, ImageLayout::eTransferSrcOptimal, ImageLayout::eShaderReadOnlyOptimal,
                                  mipmap, 0, layers);

            cmdBuff.end();
            renderer->transferQueue.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff));
            renderer->transferQueue.waitIdle();

            renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPoolTransfer, 1, &cmdBuff);
        }
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.create"));
    }
}

auto OMRendererTextureVk::acquireUpdateSlot(uint64_t bytes) -> OMTextureUpdateSlot &
{
    if (updateSlot.inFlight && updateSlot.fence)
    {
        renderer->logicalDevice.waitForFences(1, &updateSlot.fence, true, std::numeric_limits<uint64_t>::max());
        updateSlot.inFlight = false;
    }

    if (!updateSlot.fence)
    {
        updateSlot.fence = renderer->logicalDevice.createFence(::vk::FenceCreateInfo(), renderer->allocator);
    }

    if (!updateSlot.cmd)
    {
        updateSlot.cmd = renderer->logicalDevice.allocateCommandBuffers(
            CommandBufferAllocateInfo(renderer->tempCommandPoolTransfer, CommandBufferLevel::ePrimary, 1))[0];
    }

    if (updateSlot.capacity < bytes)
    {
        delete updateSlot.staging;
        updateSlot.staging = renderer->allocateBuffer(common::Misc, bytes);
        updateSlot.capacity = bytes;
    }

    return updateSlot;
}

OMRendererTextureVk::~OMRendererTextureVk()
{
    try
    {
        renderer->logicalDevice.destroySampler(sampler, renderer->allocator);
        renderer->logicalDevice.destroyImageView(imageView, renderer->allocator);
        renderer->logicalDevice.freeMemory(imageMemory, renderer->allocator);
        renderer->logicalDevice.destroyImage(image, renderer->allocator);

        delete updateSlot.staging;
        updateSlot.staging = nullptr;
        renderer->logicalDevice.destroyFence(updateSlot.fence, renderer->allocator);
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

static auto toStage(ImageLayout layout) -> PipelineStageFlags
{
    switch (layout)
    {
    case ImageLayout::eTransferSrcOptimal:
    case ImageLayout::eTransferDstOptimal:
        return PipelineStageFlagBits::eTransfer;
    case ImageLayout::eShaderReadOnlyOptimal:
        return PipelineStageFlagBits::eFragmentShader;
    case ImageLayout::eDepthStencilAttachmentOptimal:
        return PipelineStageFlagBits::eFragmentShader | PipelineStageFlagBits::eVertexShader;
    case ImageLayout::eUndefined:
    default:
        return PipelineStageFlagBits::eTopOfPipe;
    }
}

void OMRendererTextureVk::transitionImageLayout(CommandBuffer cmd, ImageLayout oldLayout, ImageLayout newLayout,
                                                uint64_t mip, uint64_t baseLayer, uint64_t layers,
                                                ::vk::ImageAspectFlags aspect)
{
    auto barrier = ImageMemoryBarrier({}, {}, oldLayout, newLayout, QueueFamilyIgnored, QueueFamilyIgnored, image,
                                      ImageSubresourceRange(aspect, mip, 1, baseLayer, layers));

    auto sourceStage = toStage(oldLayout), destinationStage = toStage(newLayout);

    barrier.srcAccessMask = toAccessMask(oldLayout);
    barrier.dstAccessMask = toAccessMask(newLayout);

    cmd.pipelineBarrier(sourceStage, destinationStage, {}, nullptr, nullptr, barrier);
}

void OMRendererTextureVk::updateData(void *p, uint64_t layer)
{
    try
    {
        const uint64_t bytes = width * height * ((this->arr == common::R8G8B8Srgb) ? 3 : 4);
        auto &slot = acquireUpdateSlot(bytes);
        slot.staging->updateData(p);

        auto cmdBuff = slot.cmd;
        cmdBuff.reset({});
        cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));
        transitionImageLayout(cmdBuff, uploaded[layer] ? ImageLayout::eShaderReadOnlyOptimal : ImageLayout::eUndefined,
                              ImageLayout::eTransferDstOptimal, 0, layer, 1);
        cmdBuff.copyBufferToImage(
            reinterpret_cast<OMRendererBufferVk *>(slot.staging)->buffer, image, ImageLayout::eTransferDstOptimal,
            BufferImageCopy(0, width, height,
                            ImageSubresourceLayers((!isColorFormat(arr)) ? ImageAspectFlagBits::eDepth
                                                                         : ImageAspectFlagBits::eColor,
                                                   0, layer, 1),
                            Offset3D(0, 0, 0), Extent3D(width, height, 1)));
        transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal, 0, layer,
                              1);

        cmdBuff.end();
        renderer->logicalDevice.resetFences(slot.fence);
        renderer->transferQueue.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff), slot.fence);
        slot.inFlight = true;

        uploaded[layer] = true;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.update"));
    }
}

void OMRendererTextureVk::updateDataMulti(const std::vector<common::OMRendererTexture::OMTextureDataUpdate> &updates)
{
    if (updates.empty())
    {
        return;
    }

    if (updates.size() == 1)
    {
        updateData(updates[0].data, updates[0].layer);
        return;
    }

    try
    {
        const uint64_t bytes = width * height * ((this->arr == common::R8G8B8Srgb) ? 3 : 4);
        auto &slot = acquireUpdateSlot(bytes * updates.size());
        auto staging = reinterpret_cast<OMRendererBufferVk *>(slot.staging);

        auto cmdBuff = slot.cmd;
        cmdBuff.reset({});
        cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));

        // 所有 layer 在 build() 阶段都已初始化过，可以对整张图用一次 barrier；
        // 若仍有未初始化的 layer（首次上传），退回逐层转换以保证正确性。
        bool allUploaded = true;
        for (auto &u : updates)
        {
            if (!uploaded[u.layer])
            {
                allUploaded = false;
                break;
            }
        }

        uint64_t offset = 0;

        if (allUploaded)
        {
            // 一次 barrier 覆盖整张图：ShaderReadOnly -> TransferDst
            transitionImageLayout(cmdBuff, ImageLayout::eShaderReadOnlyOptimal, ImageLayout::eTransferDstOptimal, 0, 0,
                                  layers);

            for (auto &u : updates)
            {
                slot.staging->updateDataPart(u.data, offset, bytes);

                cmdBuff.copyBufferToImage(
                    staging->buffer, image, ImageLayout::eTransferDstOptimal,
                    BufferImageCopy(offset, width, height,
                                    ImageSubresourceLayers((!isColorFormat(arr)) ? ImageAspectFlagBits::eDepth
                                                                                 : ImageAspectFlagBits::eColor,
                                                           0, u.layer, 1),
                                    Offset3D(0, 0, 0), Extent3D(width, height, 1)));

                uploaded[u.layer] = true;
                offset += bytes;
            }

            // 一次 barrier 覆盖整张图：TransferDst -> ShaderReadOnly
            transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal, 0, 0,
                                  layers);
        }
        else
        {
            for (auto &u : updates)
            {
                slot.staging->updateDataPart(u.data, offset, bytes);

                transitionImageLayout(cmdBuff,
                                      uploaded[u.layer] ? ImageLayout::eShaderReadOnlyOptimal : ImageLayout::eUndefined,
                                      ImageLayout::eTransferDstOptimal, 0, u.layer, 1);
                cmdBuff.copyBufferToImage(
                    staging->buffer, image, ImageLayout::eTransferDstOptimal,
                    BufferImageCopy(offset, width, height,
                                    ImageSubresourceLayers((!isColorFormat(arr)) ? ImageAspectFlagBits::eDepth
                                                                                 : ImageAspectFlagBits::eColor,
                                                           0, u.layer, 1),
                                    Offset3D(0, 0, 0), Extent3D(width, height, 1)));
                transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal, 0,
                                      u.layer, 1);

                uploaded[u.layer] = true;
                offset += bytes;
            }
        }

        cmdBuff.end();
        renderer->logicalDevice.resetFences(slot.fence);
        renderer->transferQueue.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff), slot.fence);
        slot.inFlight = true;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.update"));
    }
}

void OMRendererTextureVk::updateDataPart(void *p, uint64_t x, uint64_t y, uint64_t w, uint64_t h, uint64_t layer)
{
    try
    {
        const uint64_t bytes = w * h * ((this->arr == common::R8G8B8Srgb) ? 3 : 4);
        auto &slot = acquireUpdateSlot(bytes);
        slot.staging->updateData(p);

        auto cmdBuff = slot.cmd;
        cmdBuff.reset({});
        cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));
        transitionImageLayout(cmdBuff, uploaded[layer] ? ImageLayout::eShaderReadOnlyOptimal : ImageLayout::eUndefined,
                              ImageLayout::eTransferDstOptimal, 0, layer, 1);
        cmdBuff.copyBufferToImage(
            reinterpret_cast<OMRendererBufferVk *>(slot.staging)->buffer, image, ImageLayout::eTransferDstOptimal,
            BufferImageCopy(0, w, h,
                            ImageSubresourceLayers((!isColorFormat(arr)) ? ImageAspectFlagBits::eDepth
                                                                         : ImageAspectFlagBits::eColor,
                                                   0, layer, 1),
                            Offset3D(x, y, 0), Extent3D(w, h, 1)));
        transitionImageLayout(cmdBuff, ImageLayout::eTransferDstOptimal, ImageLayout::eShaderReadOnlyOptimal, 0, layer,
                              1);

        cmdBuff.end();
        renderer->logicalDevice.resetFences(slot.fence);
        renderer->transferQueue.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff), slot.fence);
        slot.inFlight = true;

        uploaded[layer] = true;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.update"));
    }
}
auto OMRendererTextureVk::fetchData(uint64_t layer) -> std::shared_ptr<void>
{
    try
    {
        const uint64_t bytes = width * height * ((this->arr == common::R8G8B8Srgb) ? 3 : 4);
        const ImageLayout currentLayout =
            uploaded[layer] ? ImageLayout::eShaderReadOnlyOptimal : ImageLayout::eUndefined;

        auto staging = renderer->allocateBuffer(common::Misc, bytes);
        auto vkStaging = reinterpret_cast<OMRendererBufferVk *>(staging);

        auto cmdBuff = renderer->logicalDevice.allocateCommandBuffers(
            CommandBufferAllocateInfo(renderer->tempCommandPoolTransfer, CommandBufferLevel::ePrimary, 1))[0];

        cmdBuff.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eOneTimeSubmit));

        transitionImageLayout(cmdBuff, currentLayout, ImageLayout::eTransferSrcOptimal, 0, layer, 1);
        cmdBuff.copyImageToBuffer(
            image, ImageLayout::eTransferSrcOptimal, vkStaging->buffer,
            BufferImageCopy(0, width, height,
                            ImageSubresourceLayers((!isColorFormat(arr)) ? ImageAspectFlagBits::eDepth
                                                                         : ImageAspectFlagBits::eColor,
                                                   0, layer, 1),
                            Offset3D(0, 0, 0), Extent3D(width, height, 1)));

        transitionImageLayout(cmdBuff, ImageLayout::eTransferSrcOptimal, ImageLayout::eShaderReadOnlyOptimal, 0, layer,
                              1);

        cmdBuff.end();
        renderer->transferQueue.submit(SubmitInfo({}, {}, {}, 1, &cmdBuff));
        renderer->transferQueue.waitIdle();

        renderer->logicalDevice.freeCommandBuffers(renderer->tempCommandPoolTransfer, 1, &cmdBuff);

        auto result = staging->fetchData();

        delete staging;

        return result;
    }
    catch (SystemError &e)
    {
        throw OMRendererException(VkErrorTranslate(e, "openminecraft.renderer.vk.err.image.fetch"));
    }
}
} // namespace openminecraft::renderer::vk
