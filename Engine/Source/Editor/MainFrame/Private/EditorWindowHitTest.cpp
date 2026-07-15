#include "EditorWindowHitTest.h"
#include "Widgets/TitleBar.h"
#include "Platform/Platform.h"

namespace we::editor::mainframe {

we::platform::WindowHitTestResult EditorWindowHitTest(
    we::platform::WindowId window,
    we::platform::Int2 area,
    void* userdata)
{
    using Result = we::platform::WindowHitTestResult;

    const auto size = we::platform::Platform::Get().GetWindowSize(window);
    const int width = static_cast<int>(size.x);
    const int height = static_cast<int>(size.y);

    constexpr int kResizeBorder = 8;
    const bool left = area.x < kResizeBorder;
    const bool right = area.x >= width - kResizeBorder;
    const bool top = area.y < kResizeBorder;
    const bool bottom = area.y >= height - kResizeBorder;

    if (top && left) return Result::ResizeTopLeft;
    if (top && right) return Result::ResizeTopRight;
    if (bottom && left) return Result::ResizeBottomLeft;
    if (bottom && right) return Result::ResizeBottomRight;
    if (left) return Result::ResizeLeft;
    if (right) return Result::ResizeRight;
    if (top) return Result::ResizeTop;
    if (bottom) return Result::ResizeBottom;

    if (userdata) {
        auto* data = static_cast<EditorWindowHitTestData*>(userdata);
        if (auto titleBar = data->titleBar.lock()) {
            return titleBar->HitTest(area);
        }
    }

    return Result::Client;
}

} // namespace we::editor::mainframe
