#pragma once

#include "ViewportEdit/Export.h"
#include "ViewportEdit/ViewportEditTypes.h"
#include "Platform/InputTypes.h"

#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace we::editor::viewportedit {

class IViewportContext;
class IViewportToolContext;

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

/// Tool-facing context — narrow DI surface so tools stay decoupled from workspace internals.
class VIEWPORTEDIT_API IViewportToolContext {
public:
    virtual ~IViewportToolContext() = default;

    [[nodiscard]] virtual IViewportContext& EditorContext() noexcept = 0;
    [[nodiscard]] virtual ViewportToolId ActiveToolId() const noexcept = 0;
    [[nodiscard]] virtual ViewportModeId ActiveModeId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view ActiveModeKey() const noexcept = 0;
    [[nodiscard]] virtual float ViewportWidth() const noexcept = 0;
    [[nodiscard]] virtual float ViewportHeight() const noexcept = 0;
};

/// Stateless-capable editing tool. Owns ephemeral interaction state only.
class VIEWPORTEDIT_API IViewportTool {
public:
    virtual ~IViewportTool() = default;

    [[nodiscard]] virtual ViewportToolId GetId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetStableId() const noexcept {
        return BuiltinToolName(GetId());
    }
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;
    [[nodiscard]] virtual ViewportToolDescriptor Describe() const {
        ViewportToolDescriptor d;
        d.id = std::string(GetStableId());
        d.displayName = std::string(GetDisplayName());
        d.builtinId = GetId();
        return d;
    }

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

/// Editor mode — hosts tools, overlays, and interaction policy for a workflow.
class VIEWPORTEDIT_API IViewportMode {
public:
    virtual ~IViewportMode() = default;

    [[nodiscard]] virtual ViewportModeId GetId() const noexcept = 0;
    [[nodiscard]] virtual std::string_view GetStableId() const noexcept {
        return BuiltinModeName(GetId());
    }
    [[nodiscard]] virtual std::string_view GetDisplayName() const noexcept = 0;
    [[nodiscard]] virtual ViewportModeDescriptor Describe() const {
        ViewportModeDescriptor d;
        d.id = std::string(GetStableId());
        d.displayName = std::string(GetDisplayName());
        d.builtinId = GetId();
        return d;
    }

    virtual void OnEnter(IViewportContext& context) = 0;
    virtual void OnExit(IViewportContext& context) = 0;
    virtual void Tick(IViewportContext& context, float deltaSeconds) = 0;

    /// Optional: mode may register/unregister its tools during enter/exit.
    [[nodiscard]] virtual std::span<const ViewportToolId> DefaultTools() const noexcept {
        return {};
    }

    [[nodiscard]] virtual ViewportToolId PreferredTool() const noexcept {
        return ViewportToolId::Select;
    }
};

} // namespace we::editor::viewportedit
