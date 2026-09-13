// ==============================================================================
// WindEffects — Editor — EditorTick
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Editor.h"
#include "Core/Logger.h"
#include "Environment/EnvironmentEditorApi.h"
#include "Environment/EnvironmentSystem.h"
#include <KindUI/EditorUI.h>
#include "Terrain/Terrain.h"
#include "Terrain/TerrainDiagnostics.h"
#include "TerrainEditor/TerrainEditor.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"

#include <chrono>

#include "Platform/UndefWin32Macros.h"

namespace we::programs::editor {

void Editor::TickSimulation(float dt) {
    we::runtime::kindui::Animator::Tick(dt);
    // Central idle gate: skip widget-tree Tick when layout/paint/animation are clean.
    // Sticky animating latch (from last MarkAnimating/Damp) keeps PeekNeedsWidgetTick
    // true across ConsumeNeedsPaint; MarkSettled clears it so Tick must re-arm.
    const bool needsWidgetTick = we::runtime::kindui::UIRepaintGate::PeekNeedsWidgetTick();
    if (needsWidgetTick) {
        we::runtime::kindui::UIRepaintGate::MarkSettled();
        if (m_RootWidget) {
            m_RootWidget->Tick(dt);
        }
    }
    if (m_Camera) {
        m_Camera->Update(dt);
    }
    if (m_Scene) {
        m_Scene->Update();
    }
    if (m_Camera) {
        auto& env = we::runtime::world::environment::EnvironmentSystem::Get();
        env.Tick(dt);
        env.SyncFromScene(m_Camera->GetPosition());
    }
    ::we::editor::environment::TickEditor();
    if (needsWidgetTick && m_WorldOutliner) {
        m_WorldOutliner->Outliner().Tick(dt);
    }
    // ContentBrowser may live outside the root tick path for service hooks; still
    // honor the same central gate (hover alpha uses MarkAnimating).
    if (needsWidgetTick && m_ContentBrowser) {
        m_ContentBrowser->Browser().Tick(dt);
    }
    if (m_PrefabEditor) {
        m_PrefabEditor->Tick(dt);
    }
    if (m_CompilationRuntime) {
        m_CompilationRuntime->Manager().Tick(dt);
    }
    if (m_ViewportEdit) {
        m_ViewportEdit->SyncSelectionFromScene();
        m_ViewportEdit->Tick(dt);
    }
    if (m_Camera) {
        const auto pos = m_Camera->GetPosition();
        const we::math::Mat4 viewProj =
            m_Camera->GetProjectionMatrix() * m_Camera->GetViewMatrix();
        we::editor::terrain::GetLandscapeEditor().Tick(
            dt, pos.x, pos.y, pos.z, &viewProj);
    }
    if (::we::editor::services::EditorPerfStats::IsPerfLoggingEnabled()
        && we::runtime::terrain::TerrainSystem::Get().IsCreated())
    {
        static double s_LastTerrainLogMs = 0.0;
        using clock = std::chrono::steady_clock;
        const double nowMs = std::chrono::duration<double, std::milli>(
            clock::now().time_since_epoch())
                                .count();
        if (nowMs - s_LastTerrainLogMs >= 1000.0) {
            HE_INFO(we::runtime::terrain::TerrainDiagnostics::Get().FormatSummary());
            s_LastTerrainLogMs = nowMs;
        }
    }
}

} // namespace we::programs::editor