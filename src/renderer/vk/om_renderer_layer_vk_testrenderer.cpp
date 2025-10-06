#include "openminecraft/renderer/vk/om_renderer_layer_vk_testrenderer.hpp"

#include "openminecraft/renderer/common/om_renderer_shader.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk.hpp"
#include "openminecraft/renderer/vk/om_renderer_layer_vk_validation.hpp"
#include "openminecraft/vfs/om_vfs_base.hpp"

#include <fstream>
#include <vulkan/vulkan_core.h>

using namespace ::vk;

namespace openminecraft::renderer::vk::test
{
uint32_t findMemoryType(uint32_t typeFilter, MemoryPropertyFlags properties, PhysicalDeviceMemoryProperties &memProperties) {
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
    {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties ) == properties)
        {
            return i;
        }
    }

    return 0;
}

OMTestRenderer::OMTestRenderer(OMRendererVk *renderer): renderer(renderer)
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

    auto attaches = std::vector{AttachmentReference(
        0,
        ImageLayout::eColorAttachmentOptimal
    )};

    auto attachments = std::vector{AttachmentDescription(
            {},
            renderer->swapchainManager->format.format,
            SampleCountFlagBits::e1,
            AttachmentLoadOp::eClear,
            AttachmentStoreOp::eStore,
            AttachmentLoadOp::eDontCare,
            AttachmentStoreOp::eDontCare,
            ImageLayout::eUndefined,
            ImageLayout::ePresentSrcKHR
        )};
    auto subpasses = std::vector{SubpassDescription(
        {},
        PipelineBindPoint::eGraphics,
        nullptr, attaches,
        nullptr
    )};
    auto depe = std::vector{SubpassDependency(
            VK_SUBPASS_EXTERNAL, 0,
            PipelineStageFlagBits::eColorAttachmentOutput, PipelineStageFlagBits::eColorAttachmentOutput,
            {}, AccessFlagBits::eColorAttachmentRead | AccessFlagBits::eColorAttachmentWrite
        )};

    renderPass = renderer->logicalDevice.createRenderPass(
        RenderPassCreateInfo(
            {},
            attachments,
            subpasses,
            depe
        ),
        renderer->allocator
    );

    pipelineLayout = renderer->logicalDevice.createPipelineLayout(PipelineLayoutCreateInfo(), renderer->allocator);

    {
        auto shaders = std::vector{
            PipelineShaderStageCreateInfo(
                {},
                ShaderStageFlagBits::eVertex,
                renderer->logicalDevice.createShaderModule(ShaderModuleCreateInfo({}, vtxShader->data.size(), reinterpret_cast<const uint32_t *>(vtxShader->data.data())), renderer->allocator),
                "main"
            ),
            PipelineShaderStageCreateInfo(
                {},
                ShaderStageFlagBits::eFragment,
                renderer->logicalDevice.createShaderModule(ShaderModuleCreateInfo({}, frgShader->data.size(), reinterpret_cast<const uint32_t *>(frgShader->data.data())), renderer->allocator),
                "main"
            )
        };

        const std::vector bi = {VertexInputBindingDescription(
                0, (2 + 3) * sizeof(float), VertexInputRate::eVertex
            )};
        const std::vector ad = {
            VertexInputAttributeDescription(
                0, 0, Format::eR32G32Sfloat, 0
            ),
            VertexInputAttributeDescription(
                1, 0, Format::eR32G32B32Sfloat, 2 * sizeof(float)
            )
        };

        auto vertexInput = PipelineVertexInputStateCreateInfo({}, bi, ad);
        auto inputAssembly = PipelineInputAssemblyStateCreateInfo({}, PrimitiveTopology::eTriangleList, false);

        const std::vector vp = {
            Viewport(
                0, 0, static_cast<float>(renderer->swapchainManager->extent.width), static_cast<float>(renderer->swapchainManager->extent.height), 0, 1
            )
        };
        const std::vector scis = {
            Rect2D(
                Offset2D(0, 0), renderer->swapchainManager->extent
            )
        };
        auto viewportState = PipelineViewportStateCreateInfo({}, vp, scis);
        auto rasterization = PipelineRasterizationStateCreateInfo({}, false, false, PolygonMode::eFill, CullModeFlagBits::eBack, FrontFace::eClockwise, true, 0, 0, 0, 1);
        auto multisample = PipelineMultisampleStateCreateInfo({}, SampleCountFlagBits::e1, false);

        const std::vector attc = {PipelineColorBlendAttachmentState(
            false, {}, {}, {}, {}, {}, {}, ColorComponentFlagBits::eA | ColorComponentFlagBits::eR | ColorComponentFlagBits::eG | ColorComponentFlagBits::eB
        )};
        auto colorblend = PipelineColorBlendStateCreateInfo({}, false, LogicOp::eCopy, attc, std::array{0.f, 0.f, 0.f, 0.f});

        auto result = renderer->logicalDevice.createGraphicsPipeline(
            {},
            GraphicsPipelineCreateInfo(
                {},
                shaders,
                &vertexInput,
                &inputAssembly,
                {},
                &viewportState,
                &rasterization,
                &multisample,
                {},
                &colorblend,
                {},
                pipelineLayout,
                renderPass,
                0,
                {},
                -1
            ),
            renderer->allocator
        );
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

    auto siz = (3 + 2) * 3 * sizeof(float);
    vertexBuffer = renderer->logicalDevice.createBuffer(BufferCreateInfo({}, siz, BufferUsageFlagBits::eVertexBuffer, SharingMode::eExclusive), renderer->allocator);

    auto req = renderer->logicalDevice.getBufferMemoryRequirements(vertexBuffer);
    auto prop = renderer->physicalDevice.getMemoryProperties();

    vertexBufferMemory = renderer->logicalDevice.allocateMemory(MemoryAllocateInfo(req.size, findMemoryType(req.memoryTypeBits, MemoryPropertyFlagBits::eHostVisible | MemoryPropertyFlagBits::eHostCoherent, prop)), renderer->allocator);

    renderer->logicalDevice.bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

    auto r = renderer->logicalDevice.mapMemory(vertexBufferMemory, 0, siz);

    float arr[] = {
        0.0f, -0.5f, 1.0f, 0.0f, 0.0f,
        0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f
    };

    std::memcpy(r, arr, sizeof(float) * 15);

    renderer->logicalDevice.unmapMemory(vertexBufferMemory);

    commandPool = renderer->logicalDevice.createCommandPool(CommandPoolCreateInfo({}, renderer->queueFamilyIndex.first), renderer->allocator);

    OMTestRenderer::reinit();
}

void OMTestRenderer::reinit()
{
    for (auto img : renderer->swapchainManager->swapchainImageViews)
    {
        framebuffers.push_back(renderer->logicalDevice.createFramebuffer(FramebufferCreateInfo({}, renderPass, img, renderer->swapchainManager->extent.width, renderer->swapchainManager->extent.height, 1), renderer->allocator));
    }

    for (auto framebuffer : framebuffers)
    {
        auto commandBuffer = renderer->logicalDevice.allocateCommandBuffers(CommandBufferAllocateInfo(commandPool, CommandBufferLevel::ePrimary, 1))[0];

        commandBuffer.begin(CommandBufferBeginInfo(CommandBufferUsageFlagBits::eSimultaneousUse));
        auto test = std::vector<ClearValue>();
        for (int i = 0; i < renderer->swapchainManager->swapchainImageViews.size(); i++)
        {
            test.push_back(ClearValue({0, 0, 0, 0}));
        }
        commandBuffer.beginRenderPass(RenderPassBeginInfo(
            renderPass, framebuffer, Rect2D(Offset2D(0, 0), renderer->swapchainManager->extent), test
        ), SubpassContents::eInline);
        commandBuffer.bindPipeline(PipelineBindPoint::eGraphics, pipeline);
        commandBuffer.bindVertexBuffers(0, std::vector{vertexBuffer}, std::vector{static_cast<DeviceSize>(0)});
        commandBuffer.draw(3, 1, 0, 0);
        commandBuffer.endRenderPass();
        commandBuffer.end();

        commandBuffers.push_back(commandBuffer);
    }
}
void OMTestRenderer::destroy()
{
    renderer->logicalDevice.freeCommandBuffers(commandPool, commandBuffers);
    renderer->logicalDevice.destroyCommandPool(commandPool, renderer->allocator);
    renderer->logicalDevice.destroyBuffer(vertexBuffer, renderer->allocator);
    renderer->logicalDevice.freeMemory(vertexBufferMemory, renderer->allocator);
    renderer->logicalDevice.destroyPipeline(pipeline, renderer->allocator);
    renderer->logicalDevice.destroyPipelineLayout(pipelineLayout, renderer->allocator);
    for (auto framebuffer : framebuffers)
    {
        renderer->logicalDevice.destroyFramebuffer(framebuffer, renderer->allocator);
    }
    renderer->logicalDevice.destroyRenderPass(renderPass, renderer->allocator);
}

}