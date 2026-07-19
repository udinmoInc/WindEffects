#pragma once

#include "ViewportEdit/Export.h"
#include "Core/Math/Types.h"

#include <cstdint>
#include <string>
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

enum class ViewportToolId : std::uint8_t {
    Select = 0,
    Move,
    Rotate,
    Scale,
    Universal,
    Measure,
    Landscape,
    Foliage,
    MeshPaint,
    Custom,
};

enum class ViewportModeId : std::uint8_t {
    Default = 0,
    Landscape,
    Foliage,
    MeshPaint,
    Animation,
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

} // namespace we::editor::viewportedit
