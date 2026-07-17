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
