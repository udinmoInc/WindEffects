#pragma once

#include "ViewportEdit/Export.h"
#include "Core/Math/Types.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace we::editor::viewportedit {

/// Opaque selection identity — maps to Scene entity Id (and later World ActorHandle).
struct VIEWPORTEDIT_API ViewportObjectId {
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const ViewportObjectId& o) const noexcept {
        return value == o.value;
    }
    [[nodiscard]] constexpr bool operator!=(const ViewportObjectId& o) const noexcept {
        return !(*this == o);
    }
};

/// Built-in tool categories. Plugin tools use Custom + stable string id.
enum class ViewportToolId : std::uint8_t {
    Select = 0,
    Move,
    Rotate,
    Scale,
    Universal,
    Measure,
    LandscapeSculpt,
    LandscapeSmooth,
    LandscapeFlatten,
    LandscapeNoise,
    LandscapePaint,
    Foliage,
    MeshPaint,
    Modeling,
    Animation,
    Physics,
    Navigation,
    Water,
    Custom,
};

/// Built-in editor modes. Plugin modes use Custom + stable string id via ModeRegistry.
enum class ViewportModeId : std::uint8_t {
    Select = 0, // alias for Default transform/select workspace
    Default = Select,
    Landscape,
    Foliage,
    MeshPaint,
    Modeling,
    Animation,
    Physics,
    Navigation,
    Water,
    MaterialPreview,
    Custom,
};

enum class TransformSpace : std::uint8_t {
    Local = 0,
    World,
    Parent,
};

enum class CameraProjection : std::uint8_t {
    Perspective = 0,
    Orthographic,
};

enum class SnapMode : std::uint8_t {
    None = 0,
    Grid,
    Rotation,
    Scale,
    Surface,
    Vertex,
    Pivot,
};

enum class SelectionOp : std::uint8_t {
    Replace = 0,
    Add,
    Toggle,
    Remove,
};

enum class ViewportInteractionPriority : std::uint8_t {
    Background = 0,
    Mode,
    Tool,
    Overlay,
    Modal,
};

enum class ViewportOverlayPass : std::uint8_t {
    World = 0,
    Gizmo,
    BrushPreview,
    Screen,
};

struct VIEWPORTEDIT_API ViewportRay {
    we::math::Vec3 origin{};
    we::math::Vec3 direction{};
};

struct VIEWPORTEDIT_API ViewportHit {
    ViewportObjectId object{};
    we::math::Vec3 worldPoint{};
    float distance = 0.f;
    bool valid = false;
};

struct VIEWPORTEDIT_API ViewportRect {
    float x = 0.f;
    float y = 0.f;
    float width = 0.f;
    float height = 0.f;

    [[nodiscard]] bool Contains(float px, float py) const noexcept {
        return px >= x && py >= y && px <= x + width && py <= y + height;
    }
};

struct VIEWPORTEDIT_API SnapSettings {
    bool gridEnabled = true;
    float gridSize = 1.f;
    bool rotationEnabled = true;
    float rotationDegrees = 15.f;
    bool scaleEnabled = false;
    float scaleStep = 0.1f;
    bool surfaceEnabled = false;
    bool vertexEnabled = false;
    bool pivotEnabled = false;
};

struct VIEWPORTEDIT_API CameraBookmark {
    std::string name;
    we::math::Vec3 position{};
    we::math::Vec3 lookAt{};
    float fovDegrees = 60.f;
    CameraProjection projection = CameraProjection::Perspective;
};

struct VIEWPORTEDIT_API ViewportEditConfig {
    bool multiSelectEnabled = true;
    bool hoverHighlightEnabled = true;
    bool outlineEnabled = true;
    float gizmoScreenSize = 96.f;
    SnapSettings snap{};
    std::uint32_t maxSelectionCount = 100'000;
};

/// Stable string identity for modes (plugin-safe). Built-ins use canonical names.
struct VIEWPORTEDIT_API ViewportModeDescriptor {
    std::string id;          // "Select", "Landscape", "Plugin.MyMode"
    std::string displayName;
    std::string iconName;
    std::string description;
    ViewportModeId builtinId = ViewportModeId::Custom;
    int sortOrder = 0;
    bool exclusive = true;
    bool loadsToolsOnEnter = true;
    bool unloadsToolsOnExit = true;
};

struct VIEWPORTEDIT_API ViewportToolDescriptor {
    std::string id; // stable string; "Select", "Landscape.Sculpt", ...
    std::string displayName;
    std::string iconName;
    std::string ownerModeId; // empty = available in all modes
    ViewportToolId builtinId = ViewportToolId::Custom;
    int sortOrder = 0;
};

[[nodiscard]] inline constexpr std::string_view BuiltinModeName(ViewportModeId id) noexcept {
    switch (id) {
    case ViewportModeId::Select: return "Select";
    case ViewportModeId::Landscape: return "Landscape";
    case ViewportModeId::Foliage: return "Foliage";
    case ViewportModeId::MeshPaint: return "MeshPaint";
    case ViewportModeId::Modeling: return "Modeling";
    case ViewportModeId::Animation: return "Animation";
    case ViewportModeId::Physics: return "Physics";
    case ViewportModeId::Navigation: return "Navigation";
    case ViewportModeId::Water: return "Water";
    case ViewportModeId::MaterialPreview: return "MaterialPreview";
    case ViewportModeId::Custom: return "Custom";
    }
    return "Custom";
}

[[nodiscard]] inline constexpr std::string_view BuiltinToolName(ViewportToolId id) noexcept {
    switch (id) {
    case ViewportToolId::Select: return "Select";
    case ViewportToolId::Move: return "Move";
    case ViewportToolId::Rotate: return "Rotate";
    case ViewportToolId::Scale: return "Scale";
    case ViewportToolId::Universal: return "Universal";
    case ViewportToolId::Measure: return "Measure";
    case ViewportToolId::LandscapeSculpt: return "Landscape.Sculpt";
    case ViewportToolId::LandscapeSmooth: return "Landscape.Smooth";
    case ViewportToolId::LandscapeFlatten: return "Landscape.Flatten";
    case ViewportToolId::LandscapeNoise: return "Landscape.Noise";
    case ViewportToolId::LandscapePaint: return "Landscape.Paint";
    case ViewportToolId::Foliage: return "Foliage";
    case ViewportToolId::MeshPaint: return "MeshPaint";
    case ViewportToolId::Modeling: return "Modeling";
    case ViewportToolId::Animation: return "Animation";
    case ViewportToolId::Physics: return "Physics";
    case ViewportToolId::Navigation: return "Navigation";
    case ViewportToolId::Water: return "Water";
    case ViewportToolId::Custom: return "Custom";
    }
    return "Custom";
}

} // namespace we::editor::viewportedit
