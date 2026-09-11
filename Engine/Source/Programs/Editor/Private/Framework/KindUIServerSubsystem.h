// ==============================================================================
// WindEffects — Editor — KindUIServerSubsystem
// Immediate-mode UI layout / late input / cursor policy for the engine loop.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/ApplicationFramework.h"
#include "Framework/IEditorLoopHost.h"

namespace we::programs::editor {

class KindUIServerSubsystem final : public we::runtime::core::ISubsystem {
public:
    explicit KindUIServerSubsystem(IEditorLoopHost& host);

    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;

    const char* GetName() const override { return "KindUIServerSubsystem"; }
    /// After application tick, before render.
    int GetPriority() const override { return 60; }

    bool LayoutOrResizeThisFrame() const { return m_LayoutOrResizeThisFrame; }
    void ResetLayoutFlag() { m_LayoutOrResizeThisFrame = false; }

private:
    IEditorLoopHost& m_Host;
    bool m_LayoutOrResizeThisFrame = false;
};

} // namespace we::programs::editor
