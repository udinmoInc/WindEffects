#pragma once

#include "Viewport/Export.hpp"

#include <memory>

namespace we::UI {
class ToolButton;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::programs::editor {

VIEWPORT_API void BindViewportCamera(const std::shared_ptr<we::runtime::engine::EditorCamera>& camera);

VIEWPORT_API void SetViewportCameraSpeedIndicator(const std::shared_ptr<we::UI::ToolButton>& indicator);
VIEWPORT_API void UpdateViewportCameraSpeedIndicator();
VIEWPORT_API void StepViewportCameraSpeed(int direction);
VIEWPORT_API void AdjustViewportCameraSpeedFromWheel(float wheelDeltaY);

VIEWPORT_API void ShowViewportCameraSpeedPopup();

VIEWPORT_API void ApplyLoadedViewportNavigationSettings();

} // namespace we::programs::editor
