// ==============================================================================
// WindEffects — CrashReporter — CrashReporterApp
// Internal implementation for the CrashReporter module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Platform/Types.h"
#include <memory>

namespace we::runtime::renderer { class Renderer; }
namespace we::runtime::kindui { class OverlayRenderer; class EventSystem; }
namespace we::programs::crashreporter { class CrashReporterUI; }

namespace we::programs::crashreporter {

class CrashReporterApp {
public:
    explicit CrashReporterApp(we::platform::WindowId window);
    ~CrashReporterApp();

    void Run();

private:
    void MainLoop();
    void Shutdown();

    we::platform::WindowId m_Window = we::platform::WindowId::Invalid;
    bool m_Running = true;

    std::shared_ptr<we::runtime::renderer::Renderer> m_Renderer;
    std::unique_ptr<we::runtime::kindui::OverlayRenderer> m_UIRenderer;
    std::shared_ptr<we::runtime::kindui::EventSystem> m_UIEventSystem;
    std::shared_ptr<CrashReporterUI> m_UI;
};

}
