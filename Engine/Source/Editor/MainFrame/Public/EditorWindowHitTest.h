#pragma once

#include "MainFrame/Export.h"
#include "Platform/Types.h"
#include <memory>

namespace WindEffects::Editor::UI {
class TitleBar;
}

namespace we::editor::mainframe {

struct EditorWindowHitTestData {
    std::weak_ptr<WindEffects::Editor::UI::TitleBar> titleBar;
};

MAINFRAME_API we::platform::WindowHitTestResult EditorWindowHitTest(
    we::platform::WindowId window,
    we::platform::Int2 area,
    void* userdata);

} // namespace we::editor::mainframe
