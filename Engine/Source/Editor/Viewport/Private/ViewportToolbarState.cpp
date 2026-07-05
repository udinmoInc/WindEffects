#include "ViewportToolbarState.hpp"
#include "ViewportNavigation.hpp"
#include "ViewportNavigationSettings.hpp"
#include "Widgets/ViewportSliderPopup.hpp"
#include "EditorCamera.hpp"
#include "Layout/OverlayManager.hpp"
#include "Widgets/ToolButton.hpp"
#include <cmath>
#include <algorithm>

namespace we::programs::editor {

namespace {
std::weak_ptr<we::runtime::engine::EditorCamera> g_Camera;
std::weak_ptr<we::UI::ToolButton> g_CameraSpeedIndicator;

void ShowPopupBelowButton(const std::shared_ptr<we::UI::Widget>& popup,
                          const std::shared_ptr<we::UI::ToolButton>& button) {
    auto overlay = we::UI::OverlayManager::Get();
    if (!overlay || !button) {
        return;
    }
    const we::UI::Rect anchor = button->GetGeometry();
    overlay->CloseAllPopups();
    overlay->ShowPopup(popup, we::UI::Point{ anchor.x, anchor.y + anchor.height + 2.0f });
}

float SnapCameraSpeed(float value) {
    return static_cast<float>(std::clamp(static_cast<int>(std::lround(value)),
        static_cast<int>(we::runtime::engine::kEditorCameraMinSpeed),
        static_cast<int>(we::runtime::engine::kEditorCameraMaxSpeed)));
}
} // namespace

void BindViewportCamera(const std::shared_ptr<we::runtime::engine::EditorCamera>& camera) {
    g_Camera = camera;
    we::UI::ApplyViewportNavigationSettings(camera);
    UpdateViewportCameraSpeedIndicator();
}

void SetViewportCameraSpeedIndicator(const std::shared_ptr<we::UI::ToolButton>& indicator) {
    g_CameraSpeedIndicator = indicator;
    UpdateViewportCameraSpeedIndicator();
}

void UpdateViewportCameraSpeedIndicator() {
    auto camera = g_Camera.lock();
    auto indicator = g_CameraSpeedIndicator.lock();
    if (!camera || !indicator) {
        return;
    }

    const int speed = static_cast<int>(std::lround(camera->GetCameraSpeed()));
    const std::string label = std::to_string(speed);
    if (indicator->GetLabel() != label) {
        indicator->SetLabel(label);
    }
}

void StepViewportCameraSpeed(int direction) {
    auto camera = g_Camera.lock();
    if (!camera || direction == 0) {
        return;
    }

    const float step = direction > 0 ? 1.0f : -1.0f;
    camera->SetCameraSpeed(camera->GetCameraSpeed() + step);
    auto& store = ViewportNavigationSettingsStore::Get();
    store.GetMutableSettings().defaultCameraSpeed = camera->GetCameraSpeed();
    store.Save();
    UpdateViewportCameraSpeedIndicator();
}

void AdjustViewportCameraSpeedFromWheel(float wheelDeltaY) {
    if (std::abs(wheelDeltaY) < 1e-4f) {
        return;
    }
    StepViewportCameraSpeed(wheelDeltaY > 0.0f ? 1 : -1);
}

void ShowViewportCameraSpeedPopup() {
    auto camera = g_Camera.lock();
    auto button = g_CameraSpeedIndicator.lock();
    if (!camera || !button) {
        return;
    }

    const float currentSpeed = camera->GetCameraSpeed();
    auto popup = std::make_shared<ViewportSliderPopup>(
        "Camera Speed",
        currentSpeed,
        we::runtime::engine::kEditorCameraMinSpeed,
        we::runtime::engine::kEditorCameraMaxSpeed,
        false,
        [](float value) { return std::to_string(static_cast<int>(std::lround(value))); },
        SnapCameraSpeed,
        [](float value) {
            if (auto cam = g_Camera.lock()) {
                cam->SetCameraSpeed(value);
                auto& store = ViewportNavigationSettingsStore::Get();
                store.GetMutableSettings().defaultCameraSpeed = value;
                store.Save();
                UpdateViewportCameraSpeedIndicator();
            }
        });

    ShowPopupBelowButton(popup, button);
}

void ApplyLoadedViewportNavigationSettings() {
    we::UI::ApplyViewportNavigationSettings(g_Camera.lock());
}

} // namespace we::programs::editor
