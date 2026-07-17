#pragma once

#include "MainFrame/Export.h"
#include "Platform/Types.h"
#include <memory>

namespace we::runtime::kindui {
class TitleBar;
}

namespace we::editor::mainframe {

struct EditorWindowHitTestData {
    std::weak_ptr<we::runtime::kindui::TitleBar> titleBar;
};

MAINFRAME_API we::platform::WindowHitTestResult EditorWindowHitTest(
    we::platform::WindowId window,
    we::platform::Int2 area,
    void* userdata);

} // namespace we::editor::mainframe
