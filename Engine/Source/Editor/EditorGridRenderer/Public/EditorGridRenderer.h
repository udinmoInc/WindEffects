// ==============================================================================
// WindEffects — EditorGridRenderer — EditorGridRenderer
// Public API surface for the EditorGridRenderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "EditorGridRenderer/Export.h"
#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <memory>

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::editor::grid {

class EDITORGRIDRENDERER_API EditorGridRenderer {
public:
    static EditorGridRenderer& Get();

    void Initialize(we::rhi::IRHIDevice* device);
    void Shutdown();

    void Render(we::rhi::IRHICommandList* commandList, const we::runtime::engine::EditorCamera& camera) const;

    bool IsInitialized() const { return m_Initialized; }

private:
    EditorGridRenderer() = default;

    we::rhi::IRHIDevice* m_Device = nullptr;
    bool m_Initialized = false;
};

} // namespace we::editor::grid
