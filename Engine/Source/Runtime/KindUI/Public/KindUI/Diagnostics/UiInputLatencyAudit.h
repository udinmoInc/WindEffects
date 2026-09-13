// ==============================================================================
// WindEffects — KindUI — UiInputLatencyAudit
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"

#include <cstdint>
#include <string>

namespace we::rhi {
struct RHIFrameStats;
}

namespace we::runtime::kindui {

/// End-to-end input→visible latency instrumentation. Enable with WE_UI_LATENCY_AUDIT=1.
enum class UiLatencyStage : uint8_t {
    OsEvent = 0,
    EventSystemReceive,
    WidgetHandler,
    Invalidation,
    Layout,
    UiBuild,
    RenderSubmit,
    GpuSubmit,
    PresentStart,
    PresentComplete,
    Count
};

enum class UiInteractionKind : uint8_t {
    Unknown = 0,
    MouseMove,
    Hover,
    Click,
    ButtonPress,
    DropdownOpen,
    DropdownSelect,
    PopupOpen,
    PopupClose,
    Drag,
    SplitterDrag,
    PanelResize,
    Scroll,
    TreeExpand,
    Selection,
    Keyboard,
    Wheel,
};

struct KINDUI_API UiLatencySample {
    UiInteractionKind kind = UiInteractionKind::Unknown;
    uint64_t traceId = 0;
    double stageMs[static_cast<size_t>(UiLatencyStage::Count)]{};
    double inputToHandlerMs = 0.0;
    double handlerToPaintMs = 0.0;
    double paintToRenderMs = 0.0;
    double renderToGpuSubmitMs = 0.0;
    double gpuSubmitToPresentMs = 0.0;
    double presentBlockMs = 0.0;
    double fenceWaitMs = 0.0;
    double frameLatencyWaitMs = 0.0;
    double paintToPresentMs = 0.0;
    double inputToPresentMs = 0.0;
    double inputToVisibleMs = 0.0;
    uint32_t inputQueueDepth = 0;
    uint32_t framesDelayed = 0;
    uint32_t refreshRateHz = 60;
    bool vsyncOn = true;
};

struct KINDUI_API UiLatencyAggregate {
    uint64_t sampleCount = 0;
    double avgInputToVisibleMs = 0.0;
    double p50InputToVisibleMs = 0.0;
    double p95InputToVisibleMs = 0.0;
    double p99InputToVisibleMs = 0.0;
    double worstInputToVisibleMs = 0.0;
    double avgUiCpuMs = 0.0;
    double avgPresentMs = 0.0;
    double avgFenceWaitMs = 0.0;
    double avgFrameLatencyWaitMs = 0.0;
    double avgInputToHandlerMs = 0.0;
    double avgHandlerToPaintMs = 0.0;
    double avgPaintToRenderMs = 0.0;
    double avgRenderToGpuSubmitMs = 0.0;
    double avgGpuSubmitToPresentMs = 0.0;
    double avgPaintToPresentMs = 0.0;
    uint32_t refreshRateHz = 0;
    bool vsyncOn = true;
};

class KINDUI_API UiInputLatencyAudit {
public:
    static UiInputLatencyAudit& Get();

    static bool IsEnabled();
    static void SetBenchmarkActive(bool active) noexcept;
    static void SetVsyncEnabled(bool enabled) noexcept;
    static void SetRefreshRateHz(uint32_t hz) noexcept;

    void BeginFrame(uint32_t inputQueueDepth);
    void OnOsEvent(UiInteractionKind kind);
    void OnEventSystemReceive();
    void OnWidgetHandler();
    void OnInvalidation();
    void OnLayout();
    void OnUiBuild();
    void OnRenderSubmit();
    void OnGpuSubmit();
    void OnPresentStart();
    void OnPresentComplete(const we::rhi::RHIFrameStats* rhiStats);

    void FlushPendingReport();

    [[nodiscard]] const UiLatencyAggregate& Aggregate() const noexcept { return m_Aggregate; }
    [[nodiscard]] const UiLatencySample& Last() const noexcept { return m_LastCompleted; }

private:
    UiInputLatencyAudit() = default;

    static double NowMs();
    void CompleteSample(const we::rhi::RHIFrameStats* rhiStats);
    void RecordSample(const UiLatencySample& sample);

    bool m_Active = false;
    UiInteractionKind m_Kind = UiInteractionKind::Unknown;
    uint64_t m_TraceId = 0;
    double m_StageTime[static_cast<size_t>(UiLatencyStage::Count)]{};
    uint32_t m_InputQueueDepth = 0;
    uint32_t m_FramesSinceInput = 0;
    bool m_VsyncOn = true;
    uint32_t m_RefreshRateHz = 60;

    UiLatencySample m_LastCompleted{};
    UiLatencyAggregate m_Aggregate{};
    static constexpr size_t kHistogramSize = 512;
    double m_Histogram[kHistogramSize]{};
    size_t m_HistogramCount = 0;
};

const char* ToString(UiInteractionKind kind);

} // namespace we::runtime::kindui
