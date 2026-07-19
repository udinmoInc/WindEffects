#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "Platform/InputTypes.h"

#include <cstdint>
#include <string_view>

namespace we::editor::viewportedit {

class IViewportContext;

enum class ViewportMouseButton : std::uint8_t {
    None = 0,
    Left,
    Right,
    Middle,
};

struct VIEWPORTEDIT_API ViewportInputEvent {
    we::platform::KeyCode key = we::platform::KeyCode::Unknown;
    ViewportMouseButton button = ViewportMouseButton::None;
    float x = 0.f;
    float y = 0.f;
    float deltaX = 0.f;
    float deltaY = 0.f;
    float scroll = 0.f;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;
};

class VIEWPORTEDIT_API IViewportTool {
public:
    virtual ~IViewportTool() = default;

    [[nodiscard]] virtual ViewportToolId GetId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;

    virtual void OnActivated(IViewportContext& context) = 0;
    virtual void OnDeactivated(IViewportContext& context) = 0;
    virtual void Tick(IViewportContext& context, float deltaSeconds) = 0;

    /// Return true if the event was consumed.
    [[nodiscard]] virtual bool OnMouseDown(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool OnMouseUp(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool OnMouseMove(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool OnKeyDown(IViewportContext& context, const ViewportInputEvent& e) = 0;
    [[nodiscard]] virtual bool OnKeyUp(IViewportContext& context, const ViewportInputEvent& e) = 0;
};

class VIEWPORTEDIT_API IViewportMode {
public:
    virtual ~IViewportMode() = default;

    [[nodiscard]] virtual ViewportModeId GetId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;

    virtual void OnEnter(IViewportContext& context) = 0;
    virtual void OnExit(IViewportContext& context) = 0;
    virtual void Tick(IViewportContext& context, float deltaSeconds) = 0;
};

} // namespace we::editor::viewportedit
