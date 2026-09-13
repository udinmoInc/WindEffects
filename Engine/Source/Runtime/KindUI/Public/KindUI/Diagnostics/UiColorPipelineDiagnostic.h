// ==============================================================================
// WindEffects — KindUI — UiColorPipelineDiagnostic
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Core/Types.h"
#include "KindUI/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <vector>

namespace we::runtime::kindui {

// Full UI color pipeline audit — enable with WE_UI_COLOR_PIPELINE_TEST=1
// Renders an isolated opaque swatch grid (no editor chrome) and readbacks swapchain pixels.
class KINDUI_API UiColorPipelineDiagnostic {
public:
    struct SwatchSpec {
        const char* hex;
        Color color;
        bool isControl = false;
    };

    struct ProbePoint {
        uint32_t x = 0;
        uint32_t y = 0;
        const SwatchSpec* swatch = nullptr;
    };

    struct StageTrace {
        Color sourceHex{};
        Color internalSrgb{};
        Color resolveColor{};
        Color paintColor{};
        Color gpuUploadLinear{};
        Color shaderInput{};
        Color shaderOutputLinear{};
        we::rhi::Format renderTargetFormat = we::rhi::Format::Unknown;
        bool renderTargetIsSrgb = false;
        const char* blendMode = nullptr;
        const char* srcAlpha = nullptr;
        const char* dstAlpha = nullptr;
        Color framebufferSrgb{};
        const char* swapchainFormat = nullptr;
        const char* finalConversion = nullptr;
        bool readbackValid = false;
        int deltaR = 0;
        int deltaG = 0;
        int deltaB = 0;
    };

    static UiColorPipelineDiagnostic& Get();
    [[nodiscard]] static bool IsEnabled();

    [[nodiscard]] static const std::vector<SwatchSpec>& CanonicalSwatches();

    void Reset();
    void BuildCpuStageTraces(we::rhi::Format targetFormat);
    void ScheduleFramebufferReadback(we::rhi::IRHIDevice* device,
                                   we::rhi::IRHICommandList* cmd,
                                   we::rhi::RHITextureHandle colorTarget,
                                   we::rhi::Format targetFormat,
                                   uint32_t width,
                                   uint32_t height);
    void TryFinalizeAndReport(we::rhi::IRHIDevice* device);

    [[nodiscard]] bool ShouldBypassEditorUi() const;
    [[nodiscard]] bool ShouldUseUiPaintOnlyPath() const;
    [[nodiscard]] bool HasReported() const { return m_Reported; }

    void AppendTestGrid(std::vector<struct UIVertex2>& vertices,
                        std::vector<uint32_t>& indices,
                        float width,
                        float height);

private:
    UiColorPipelineDiagnostic() = default;

    void PrintReport() const;
    [[nodiscard]] StageTrace TraceSwatch(const SwatchSpec& swatch, we::rhi::Format targetFormat) const;

    bool m_Reported = false;
    bool m_ReadbackScheduled = false;
    bool m_CpuTracesBuilt = false;
    uint32_t m_Width = 0;
    uint32_t m_Height = 0;
  we::rhi::Format m_TargetFormat = we::rhi::Format::Unknown;
    std::vector<StageTrace> m_Traces;
    std::vector<ProbePoint> m_Probes;

    we::rhi::RHIBufferHandle m_ReadbackBuffer = we::rhi::RHIBufferHandle::Invalid;
    uint64_t m_ReadbackCapacity = 0;
    uint32_t m_PendingProbeCount = 0;
};

} // namespace we::runtime::kindui
