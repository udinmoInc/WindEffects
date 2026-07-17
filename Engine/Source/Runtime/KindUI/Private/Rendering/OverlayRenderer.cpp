#include "KindUI/Rendering/OverlayRenderer.h"

#include "KindUI/Rendering/IconRenderer.h"
#include "KindUI/Rendering/Icons/IconManager.h"
#include "KindUI/Rendering/TextUIService.h"
#include "KindUI/Rendering/UIWidgetAdapter.h"
#include "KindUI/Rendering/UIStateManager.h"
#include "KindUI/Rendering/UiGpuUpload.h"
#include "Rendering/UiImmediateRenderer.h"

#include "Core/AssetRegistry.h"
#include "Core/FrameCounter.h"
#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Core/Widget.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

namespace we::runtime::kindui {
namespace {

[[nodiscard]] we::rhi::UIDrawList BuildDrawList(
    const std::vector<UIVertex2>& vertices,
    const std::vector<uint32_t>& indices,
    const std::vector<UIRenderBatch>& batches,
    uint32_t width,
    uint32_t height)
{
    we::rhi::UIDrawList list{};
    list.targetWidth = width;
    list.targetHeight = height;
    list.vertices.resize(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        auto& dst = list.vertices[i];
        const auto& src = vertices[i];
        std::memcpy(dst.position, src.position, sizeof(dst.position));
        std::memcpy(dst.uv, src.uv, sizeof(dst.uv));
        std::memcpy(dst.color, src.color, sizeof(dst.color));
        std::memcpy(dst.sdfRect, src.sdfRect, sizeof(dst.sdfRect));
        std::memcpy(dst.sdfParams, src.sdfParams, sizeof(dst.sdfParams));
    }
    list.indices = indices;
    list.batches.reserve(batches.size());
    for (const auto& batch : batches) {
        we::rhi::UIDrawBatch out{};
        out.texture = batch.textureSet;
        out.indexCount = batch.indexCount;
        out.firstIndex = batch.firstIndex;
        out.vertexOffset = batch.vertexOffset;
        std::memcpy(out.scissor, batch.scissor, sizeof(out.scissor));
        out.stencilRef = batch.stencilRef;
        out.isText = batch.isText;
        out.atlasWidth = batch.atlasWidth;
        out.atlasHeight = batch.atlasHeight;
        out.msdfPixelRange = batch.msdfPixelRange;
        list.batches.push_back(out);
    }
    return list;
}

} // namespace

OverlayRenderer::OverlayRenderer() = default;

OverlayRenderer::~OverlayRenderer() {
    Shutdown();
}

bool OverlayRenderer::Init(we::rhi::IRHIDevice* device, we::rhi::Format swapchainFormat, uint32_t maxFramesInFlight) {
    if (!device) {
        return false;
    }
    m_RHIDevice = device;
    m_SwapchainFormat = swapchainFormat;
    m_MaxFramesInFlight = maxFramesInFlight ? maxFramesInFlight : 2;

    m_UIImmediate = std::make_unique<UiImmediateRenderer>();
    if (!m_UIImmediate->Init(device, swapchainFormat, m_MaxFramesInFlight)) {
        WE_LOG_WARN(we::LogCategory::Startup,
            "OverlayRenderer: UiImmediateRenderer Init failed; UI will not record GPU draws.");
        m_UIImmediate.reset();
    }

    if (m_UIImmediate) {
        m_DummyDescriptorSet = static_cast<uint64_t>(m_UIImmediate->GetDummyTexture());
        m_DummySampler = static_cast<uint64_t>(m_UIImmediate->GetDefaultSampler());
    } else {
        m_DummyDescriptorSet = 1;
        m_DummySampler = 1;
    }

    m_GpuUpload = std::make_unique<UiGpuUpload>();
    m_GpuUpload->Init(device);
    m_TextUIService = std::make_unique<TextUIService>();
    (void)m_TextUIService->Initialize(this);
    m_IconRenderer = std::make_unique<IconRenderer>();
    m_IconManager = std::make_unique<IconManager>();

    const auto metaPath = we::core::AssetRegistry::ResolveAssetPath({
        "Assets/Icons/Atlas/icons.weiconmeta",
        "Icons/Atlas/icons.weiconmeta",
        "../Assets/Icons/Atlas/icons.weiconmeta",
    });
    const auto atlasRoot = [&]() -> std::filesystem::path {
        if (!metaPath.empty()) {
            return std::filesystem::path(metaPath).parent_path();
        }
        const auto probe = we::core::AssetRegistry::ResolveAssetPath({
            "Assets/Icons/Atlas/ui_Atlas_16.weiconatlas",
            "Icons/Atlas/ui_Atlas_16.weiconatlas",
        });
        return probe.empty() ? std::filesystem::path{} : std::filesystem::path(probe).parent_path();
    }();
    if (!metaPath.empty() && !atlasRoot.empty()) {
        if (m_IconManager->Init(this, metaPath, atlasRoot)) {
            m_IconRenderer->SetIconManager(m_IconManager.get());
        } else {
            WE_LOG_WARN(we::LogCategory::Startup, "OverlayRenderer: IconManager init failed.");
        }
    } else {
        WE_LOG_WARN(we::LogCategory::Startup, "OverlayRenderer: icon atlas assets not found.");
    }

    m_WidgetAdapter = std::make_unique<UIWidgetAdapter>();
    m_WidgetAdapter->Initialize(this);
    m_StateManager = std::make_unique<UIStateManager>();

    WE_LOG_INFO(we::LogCategory::Startup,
        m_UIImmediate
            ? "OverlayRenderer initialized (geometry + UiImmediateRenderer/IRHI)."
            : "OverlayRenderer initialized (CPU geometry only; GPU UI unavailable).");
    return true;
}

void OverlayRenderer::Shutdown() {
    if (m_WidgetAdapter) {
        m_WidgetAdapter->Shutdown();
    }
    if (m_GpuUpload) {
        m_GpuUpload->Shutdown();
    }
    if (m_UIImmediate) {
        m_UIImmediate->Shutdown();
        m_UIImmediate.reset();
    }
    m_RHIDevice = nullptr;
    m_Vertices.clear();
    m_Indices.clear();
    m_Batches.clear();
}

void OverlayRenderer::SetTargetExtent(uint32_t width, uint32_t height) {
    m_CurrentWidth = width;
    m_CurrentHeight = height;
    m_FrameStats.width = width;
    m_FrameStats.height = height;
}

void OverlayRenderer::RenderUI(const std::shared_ptr<Widget>& root, uint32_t frameSlot) {
    m_ActiveFrameSlot = frameSlot;
    const uint64_t frameNumber = we::runtime::core::FrameCounter::GetFrameNumber();
    const uint32_t width = m_CurrentWidth;
    const uint32_t height = m_CurrentHeight;

    if (!root || width == 0 || height == 0) {
        m_Vertices.clear();
        m_Indices.clear();
        m_Batches.clear();
        return;
    }

    const bool sizeChanged = width != m_LastBuiltWidth || height != m_LastBuiltHeight;
    const bool forceRebuild = frameNumber <= 3 || sizeChanged || m_Vertices.empty();
    const bool rebuild = forceRebuild || UIRepaintGate::ConsumeNeedsRebuild();

    if (rebuild) {
        Widget::ResetDiagnostics();
        if (m_WidgetAdapter) {
            m_WidgetAdapter->ResetDiagnostics();
            m_WidgetAdapter->ProcessWidget(root, width, height);
            m_Vertices = m_WidgetAdapter->GetVertices();
            m_Indices = m_WidgetAdapter->GetIndices();
            m_Batches = m_WidgetAdapter->GetBatches();
        }
        m_LastBuiltWidth = width;
        m_LastBuiltHeight = height;
        ++m_GeometryGeneration;
    }

    m_FrameStats.vertices = static_cast<uint32_t>(m_Vertices.size());
    m_FrameStats.indices = static_cast<uint32_t>(m_Indices.size());
    m_FrameStats.batches = static_cast<uint32_t>(m_Batches.size());
    m_FrameStats.drawCalls = m_FrameStats.batches;
    m_FrameStats.width = width;
    m_FrameStats.height = height;
}

void OverlayRenderer::BeginOverlayPass(const we::runtime::uigfx::OverlayRenderContext& context) {
    m_PendingContext = context;
    if (context.targetExtent.width != 0 && context.targetExtent.height != 0) {
        m_CurrentWidth = context.targetExtent.width;
        m_CurrentHeight = context.targetExtent.height;
    }
}

void OverlayRenderer::EndOverlayPass(const we::runtime::uigfx::OverlayRenderContext& context) {
    if (!m_UIImmediate || m_Vertices.empty() || m_Batches.empty()) {
        return;
    }
    if (!context.cmd) {
        WE_LOG_WARN(we::LogCategory::Renderer.data(),
            "OverlayRenderer::EndOverlayPass: no command list available; skipping UI draw.");
        return;
    }

    we::rhi::FramePresentParams params{};
    params.commandList = context.cmd;
    params.targetView = context.targetView;
    params.colorTarget = context.colorTarget;
    params.extent = context.targetExtent;
    params.imageIndex = context.imageIndex;

    m_UIImmediate->BeginFrame(params);
    m_UIImmediate->SubmitDrawList(
        BuildDrawList(m_Vertices, m_Indices, m_Batches, m_CurrentWidth, m_CurrentHeight),
        m_ActiveFrameSlot);
    m_UIImmediate->EndFrame();
}

void OverlayRenderer::SetPipelineAuditImageIndex(uint32_t imageIndex) {
    m_PipelineAuditImageIndex = imageIndex;
}

we::rhi::RHIDescriptorSetHandle OverlayRenderer::RegisterTexture(
    we::rhi::RHITextureViewHandle imageView,
    we::rhi::RHISamplerHandle sampler)
{
    if (m_UIImmediate) {
        return m_UIImmediate->RegisterTexture(imageView, sampler);
    }
    return static_cast<we::rhi::RHIDescriptorSetHandle>(++m_DummyDescriptorSet);
}

void OverlayRenderer::UpdateTexture(
    we::rhi::RHIDescriptorSetHandle descriptorSet,
    we::rhi::RHITextureViewHandle imageView,
    we::rhi::RHISamplerHandle sampler)
{
    if (m_UIImmediate) {
        m_UIImmediate->UpdateTexture(descriptorSet, imageView, sampler);
    }
}

void OverlayRenderer::UnregisterTexture(we::rhi::RHIDescriptorSetHandle descriptorSet) {
    if (m_UIImmediate) {
        m_UIImmediate->UnregisterTexture(descriptorSet);
    }
}

we::rhi::RHIDescriptorSetHandle OverlayRenderer::UploadRgbaTexture(
    uint32_t width,
    uint32_t height,
    std::span<const uint8_t> rgba,
    bool linearFilter)
{
    if (m_UIImmediate) {
        return m_UIImmediate->UploadRgbaTexture(width, height, rgba, linearFilter);
    }
    return we::rhi::RHIDescriptorSetHandle::Invalid;
}

TextUIService* OverlayRenderer::GetTextUIService() const { return m_TextUIService.get(); }
IconRenderer* OverlayRenderer::GetIconRenderer() const { return m_IconRenderer.get(); }
IconManager* OverlayRenderer::GetIconManager() const { return m_IconManager.get(); }

} // namespace we::runtime::kindui
