// ==============================================================================
// WindEffects — KindUI — UICommandBuffer
// Public API surface for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <cstdint>
#include <vector>

namespace we::runtime::kindui {

struct UIRenderBatch;
struct UIVertex2;

class KINDUI_API UICommandBuffer {
public:
    UICommandBuffer() = default;
    ~UICommandBuffer() = default;

    void Initialize(we::rhi::IRHIDevice* device, uint32_t maxFramesInFlight);
    void Shutdown();

    void BeginRecording(uint32_t frameIndex);
    void RecordDraw(const UIRenderBatch& batch,
                    const std::vector<UIVertex2>& vertices,
                    const std::vector<uint32_t>& indices);
    void EndRecording();

    void Execute(we::rhi::IRHICommandList* cmd);

    uint32_t GetDrawCallCount() const { return m_DrawCallCount; }

private:
    we::rhi::IRHIDevice* m_Device = nullptr;
    uint32_t m_MaxFramesInFlight = 0;
    uint32_t m_DrawCallCount = 0;
    uint32_t m_ActiveFrame = 0;
};

} // namespace we::runtime::kindui
