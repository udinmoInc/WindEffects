#pragma once

#include "Viewport/Export.h"

#include <memory>

namespace WindEffects::Editor::UI {
class ToolButton;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::programs::editor {

VIEWPORT_API void BindViewportCamera(const std::shared_ptr<we::runtime::engine::EditorCamera>& camera);

VIEWPORT_API void SetViewportCameraSpeedIndicator(const std::shared_ptr<WindEffects::Editor::UI::ToolButton>& indicator);
VIEWPORT_API void UpdateViewportCameraSpeedIndicator();
VIEWPORT_API void StepViewportCameraSpeed(int direction);
VIEWPORT_API void AdjustViewportCameraSpeedFromWheel(float wheelDeltaY);

VIEWPORT_API void ShowViewportCameraSpeedPopup();

VIEWPORT_API void ApplyLoadedViewportNavigationSettings();

} // namespace we::programs::editor
