// ==============================================================================
// WindEffects — Editor — EditorMainLoop
// Thin entry that drives IEngineLoop via EditorApplicationFramework.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Editor.h"
#include "Framework/EditorApplicationFramework.h"
#include "Core/DiagnosticMacros.h"
#include "Core/EngineWatchdog.h"
#include "Core/LoopExecutionTrace.h"
#include "Platform/PlatformSDK.h"

#include "Platform/UndefWin32Macros.h"

namespace we::programs::editor {

void Editor::MainLoop() {
    we::runtime::core::EngineWatchdog::Get().Heartbeat("Editor.MainLoop.Start");
    we::runtime::core::LoopExecutionTrace::Event(
        "Editor.MainLoop",
        we::runtime::core::LoopExecutionTrace::IsEnabled()
            ? "WE_LOOP_TRACE=1 active"
            : "WE_LOOP_TRACE off (set env to enable)");

    m_ApplicationFramework = std::make_unique<EditorApplicationFramework>(*this);

    auto& platform = we::platform::Platform::Get();
    uint64_t lastTime = platform.GetHighResolutionCounter();
    const double frequency = static_cast<double>(platform.GetHighResolutionFrequency());

    {
        we::runtime::core::EngineWatchdog::Scoped watchdogInit("Editor.MainLoop.Initialize");
        we::runtime::core::LoopExecutionTrace::Scoped initScope("Editor.MainLoop.Initialize");
        m_ApplicationFramework->Initialize();
    }

    while (m_ApplicationFramework->IsRunning() && m_Running) {
        we::runtime::core::EngineWatchdog::Scoped watchdogTick("Editor.MainLoop.Tick");
        we::runtime::core::LoopExecutionTrace::Scoped tickScope("Editor.MainLoop.Tick");
        uint64_t now = platform.GetHighResolutionCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;
        if (dt > 0.1f) {
            dt = 0.1f;
        }

        if (!m_ApplicationFramework->Tick(dt)) {
            we::runtime::core::LoopExecutionTrace::Event("Editor.MainLoop.TickBreak", "Tick returned false");
            break;
        }
    }

    {
        we::runtime::core::LoopExecutionTrace::Scoped shutdownScope("Editor.MainLoop.Shutdown");
        m_ApplicationFramework->Shutdown();
    }
    m_ApplicationFramework.reset();
    HE_INFO("[Editor] Main loop exited (framework-driven IEngineLoop).");
}

} // namespace we::programs::editor
