// ==============================================================================
// WindEffects — KindUI — UiColorCompositionDiagnostic
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Export.h"
#include "KindUI/Theme/DesignToken.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace we::runtime::kindui { class Widget; }

namespace we::runtime::kindui {

// Phase 2 — real editor composition audit. Enable with WE_UI_COLOR_COMPOSITION_TEST=1
class KINDUI_API UiColorCompositionDiagnostic {
public:
    struct LayerSample {
        DrawCommandType type = DrawCommandType::Rect;
        Color color{};
        float borderRadius = 0.0f;
        float thickness = 0.0f;
        float blur = 0.0f;
        Rect rect{};
        const char* blendMode = "SrcAlpha/OneMinusSrcAlpha";
        const char* role = "unknown";
    };

    struct SurfaceProbe {
        std::string name;
        ColorToken expectedToken = ColorToken::PanelBackground;
        Rect sampleCenterRect{};
        Rect sampleEdgeRect{};
        Rect widgetRect{};
        bool valid = false;
    };

    struct SurfaceReport {
        SurfaceProbe probe{};
        Color resolvedTokenColor{};
        Color framebufferCenter{};
        Color framebufferEdge{};
        bool centerReadbackValid = false;
        bool edgeReadbackValid = false;
        int deltaCenterR = 0;
        int deltaCenterG = 0;
        int deltaCenterB = 0;
        std::vector<LayerSample> centerLayers;
        std::vector<LayerSample> edgeLayers;
        char mismatchCategory = '?';
    };

    struct TraceChain {
        std::string surface;
        Color resolveColor{};
        Color paintCommand{};
        Color gpuUploadLinear{};
        Color shaderOutputLinear{};
        Color framebuffer{};
        std::string notes;
    };

    using ProbeRegistrarFn = std::function<void(const std::shared_ptr<Widget>&)>;

    static UiColorCompositionDiagnostic& Get();
    [[nodiscard]] static bool IsEnabled();
    static void SetProbeRegistrar(ProbeRegistrarFn registrar);
    static void InvokeProbeRegistrar(const std::shared_ptr<Widget>& root);

    void Reset();
    void BeginFrame(uint32_t width, uint32_t height);
    void RecordDrawCommands(const std::vector<DrawCommand>& commands);
    void RegisterProbe(SurfaceProbe probe);
    void ClearProbes();
    void SetPanelAbTarget(const Rect& bodyRect);
    [[nodiscard]] bool ShouldInjectPanelFlatOverride() const;
    [[nodiscard]] Rect GetPanelFlatOverrideRect() const;
    void NotifyPanelFlatOverrideInjected();

    void ScheduleFramebufferReadback(we::rhi::IRHIDevice* device,
                                   we::rhi::IRHICommandList* cmd,
                                   we::rhi::RHITextureHandle colorTarget,
                                   we::rhi::Format targetFormat,
                                   uint32_t width,
                                   uint32_t height);

    void OnFramePresented(we::rhi::IRHIDevice* device);
    [[nodiscard]] bool HasCompleted() const { return m_Completed; }
    [[nodiscard]] bool IsProbesCached() const { return m_ProbesCached; }
    void SetProbesCached(bool cached) { m_ProbesCached = cached; }
    void InvalidateProbes() { m_ProbesCached = false; }

private:
    UiColorCompositionDiagnostic() = default;

    void BuildReports();
    void PrintReport() const;
    [[nodiscard]] SurfaceReport AnalyzeProbe(const SurfaceProbe& probe) const;
    [[nodiscard]] std::vector<LayerSample> CollectLayersAt(const Point& p) const;
    [[nodiscard]] char ClassifyMismatch(const SurfaceReport& report) const;
    void PrintTraceChain(const std::string& surface, ColorToken token, const SurfaceReport& report) const;
    void PrintAbComparison() const;

    struct ReadbackPoint {
        size_t reportIndex = 0;
        bool edge = false;
        uint32_t x = 0;
        uint32_t y = 0;
    };

    bool m_Completed = false;
    bool m_ProbesCached = false;
    bool m_ReadbackScheduled = false;
    uint32_t m_Frame = 0;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
    we::rhi::Format m_TargetFormat = we::rhi::Format::Unknown;

    std::vector<DrawCommand> m_Commands;
    uint32_t m_LastRecordedCommandCount = 0;
    std::vector<SurfaceProbe> m_Probes;
    std::vector<SurfaceReport> m_Reports;

    Rect m_PanelAbTarget{};
    bool m_PanelAbTargetValid = false;
    bool m_InjectFlatOverride = false;
    bool m_FlatOverrideInjected = false;
    Color m_ProductionPanelCenter{};
    Color m_FlatOverridePanelCenter{};
    bool m_HasProductionPanelSample = false;
    bool m_HasFlatOverrideSample = false;

    we::rhi::RHIBufferHandle m_ReadbackBuffer = we::rhi::RHIBufferHandle::Invalid;
    uint64_t m_ReadbackCapacity = 0;
    uint32_t m_PendingReadbackCount = 0;
    std::vector<ReadbackPoint> m_ReadbackPointMap;

    static ProbeRegistrarFn s_ProbeRegistrar;
};

} // namespace we::runtime::kindui
