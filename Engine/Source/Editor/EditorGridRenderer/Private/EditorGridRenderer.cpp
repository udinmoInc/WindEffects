// ==============================================================================
// WindEffects — EditorGridRenderer — EditorGridRenderer
// Internal implementation for the EditorGridRenderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "EditorGridRenderer.h"

namespace we::editor::grid {

EditorGridRenderer& EditorGridRenderer::Get() {
    static EditorGridRenderer instance;
    return instance;
}

void EditorGridRenderer::Initialize(we::rhi::IRHIDevice* device) {
    m_Device = device;
    m_Initialized = m_Device != nullptr;
}

void EditorGridRenderer::Shutdown() {
    m_Device = nullptr;
    m_Initialized = false;
}

void EditorGridRenderer::Render(we::rhi::IRHICommandList*, const we::runtime::engine::EditorCamera&) const {}

} // namespace we::editor::grid
