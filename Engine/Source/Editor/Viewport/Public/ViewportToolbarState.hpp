#pragma once

#include <memory>

namespace we::UI {
class ToolButton;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::programs::editor {

void BindViewportCamera(const std::shared_ptr<we::runtime::engine::EditorCamera>& camera);

void SetViewportCameraSpeedIndicator(const std::shared_ptr<we::UI::ToolButton>& indicator);
void UpdateViewportCameraSpeedIndicator();
void StepViewportCameraSpeed(int direction);
void AdjustViewportCameraSpeedFromWheel(float wheelDeltaY);

void ShowViewportCameraSpeedPopup();

} // namespace we::programs::editor
