// ==============================================================================
// WindEffects — MainFrame — EditorWindowHitTest
// Public API surface for the MainFrame module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "MainFrame/Export.h"
#include "Platform/Types.h"
#include <memory>

namespace we::editor::shell {
class TitleBar;
}

namespace we::editor::mainframe {

struct EditorWindowHitTestData {
    std::weak_ptr<::we::editor::shell::TitleBar> titleBar;
};

MAINFRAME_API we::platform::WindowHitTestResult EditorWindowHitTest(
    we::platform::WindowId window,
    we::platform::Int2 area,
    void* userdata);

} // namespace we::editor::mainframe
