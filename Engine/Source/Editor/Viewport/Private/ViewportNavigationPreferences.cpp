#include "ViewportNavigationPreferences.h"
#include "WindEffects/Editor/UI/Shell/ViewportNavigationSettings.h"
#include "ViewportNavigation.h"
#include "ViewportToolbarState.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "WindEffects/Editor/EditorSDK.h"

#include "Widgets/ViewportSliderPopup.h"
#include "WindEffects/Editor/UI/Widgets/Panel.h"
#include "KindUI/Layout/Flex.h"
#include "Widgets/ToolButton.h"
#include "KindUI/Core/Icon.h"

#include <memory>
#include <string>

namespace we::programs::editor {

namespace {

std::shared_ptr<we::runtime::kindui::Column> BuildViewportNavigationPreferencesContent() {
    auto content = std::make_shared<we::runtime::kindui::Column>();
    content->Padding(we::runtime::kindui::Margin{ 12.0f, 12.0f, 12.0f, 12.0f });
    content->Gap(6.0f);

    auto& store = ViewportNavigationSettingsStore::Get();
    store.EnsureLoaded();

    auto addSliderRow = [&](const std::string& label,
                            float value,
                            float minValue,
                            float maxValue,
                            auto onChanged) {
        auto row = std::make_shared<we::runtime::kindui::ToolButton>(
            we::runtime::kindui::Icons::SettingsName,
            label + ": " + std::to_string(value).substr(0, 5),
            [value, minValue, maxValue, onChanged, label]() {
                auto popup = std::make_shared<ViewportSliderPopup>(
                    label,
                    value,
                    minValue,
                    maxValue,
                    false,
                    [](float v) { return std::to_string(v).substr(0, 5); },
                    [](float v) { return v; },
                    [onChanged](float v) {
                        onChanged(v);
                    });
                if (auto* overlay = GetEditorPopupHost()) {
                    overlay->CloseAllPopups();
                    overlay->ShowPopup(popup, we::runtime::kindui::Point{ 140.0f, 120.0f });
                }
            },
            "Edit " + label);
        content->AddChild(row);
    };

    const auto& settings = store.GetSettings();
    addSliderRow("Mouse Sensitivity", settings.mouseSensitivity, 0.01f, 1.0f, [&](float v) {
        store.GetMutableSettings().mouseSensitivity = v;
        store.GetMutableSettings().preset = NavigationPreset::Custom;
        store.Save();
        ApplyLoadedViewportNavigationSettings();
    });
    addSliderRow("Camera Acceleration", settings.cameraAcceleration, 0.1f, 4.0f, [&](float v) {
        store.GetMutableSettings().cameraAcceleration = v;
        store.GetMutableSettings().preset = NavigationPreset::Custom;
        store.Save();
        ApplyLoadedViewportNavigationSettings();
    });
    addSliderRow("Camera Smoothing", settings.cameraSmoothing, 0.0f, 30.0f, [&](float v) {
        store.GetMutableSettings().cameraSmoothing = v;
        store.GetMutableSettings().preset = NavigationPreset::Custom;
        store.Save();
        ApplyLoadedViewportNavigationSettings();
    });
    addSliderRow("Default Camera Speed", settings.defaultCameraSpeed, 1.0f, 50.0f, [&](float v) {
        store.GetMutableSettings().defaultCameraSpeed = v;
        store.GetMutableSettings().preset = NavigationPreset::Custom;
        store.Save();
        ApplyLoadedViewportNavigationSettings();
        UpdateViewportCameraSpeedIndicator();
    });
    addSliderRow("Max Boost Multiplier", settings.maxBoostMultiplier, 1.0f, 16.0f, [&](float v) {
        store.GetMutableSettings().maxBoostMultiplier = v;
        store.GetMutableSettings().preset = NavigationPreset::Custom;
        store.Save();
        ApplyLoadedViewportNavigationSettings();
    });
    addSliderRow("Scroll Wheel Speed Multiplier", settings.scrollWheelSpeedMultiplier, 0.1f, 5.0f, [&](float v) {
        store.GetMutableSettings().scrollWheelSpeedMultiplier = v;
        store.GetMutableSettings().preset = NavigationPreset::Custom;
        store.Save();
        ApplyLoadedViewportNavigationSettings();
    });

    auto presetBtn = std::make_shared<we::runtime::kindui::ToolButton>(
        we::runtime::kindui::Icons::SettingsName,
        "Preset: " + ViewportNavigationSettingsStore::PresetToString(settings.preset),
        []() {
            auto& navStore = ViewportNavigationSettingsStore::Get();
            const auto current = navStore.GetSettings().preset;
            NavigationPreset next = NavigationPreset::UE5;
            switch (current) {
            case NavigationPreset::UE5: next = NavigationPreset::Blender; break;
            case NavigationPreset::Blender: next = NavigationPreset::Maya; break;
            case NavigationPreset::Maya: next = NavigationPreset::Unity; break;
            case NavigationPreset::Unity: next = NavigationPreset::UE5; break;
            case NavigationPreset::Custom: next = NavigationPreset::UE5; break;
            }
            navStore.ApplyPreset(next);
            navStore.Save();
            ApplyLoadedViewportNavigationSettings();
            UpdateViewportCameraSpeedIndicator();
        },
        "Cycle navigation preset");
    content->AddChild(presetBtn);

    auto invertX = std::make_shared<we::runtime::kindui::ToolButton>(
        we::runtime::kindui::Icons::SettingsName,
        std::string("Invert X: ") + (settings.invertX ? "On" : "Off"),
        []() {
            auto& navStore = ViewportNavigationSettingsStore::Get();
            navStore.GetMutableSettings().invertX = !navStore.GetSettings().invertX;
            navStore.GetMutableSettings().preset = NavigationPreset::Custom;
            navStore.Save();
            ApplyLoadedViewportNavigationSettings();
        },
        "Toggle invert X");
    content->AddChild(invertX);

    auto invertY = std::make_shared<we::runtime::kindui::ToolButton>(
        we::runtime::kindui::Icons::SettingsName,
        std::string("Invert Y: ") + (settings.invertY ? "On" : "Off"),
        []() {
            auto& navStore = ViewportNavigationSettingsStore::Get();
            navStore.GetMutableSettings().invertY = !navStore.GetSettings().invertY;
            navStore.GetMutableSettings().preset = NavigationPreset::Custom;
            navStore.Save();
            ApplyLoadedViewportNavigationSettings();
        },
        "Toggle invert Y");
    content->AddChild(invertY);

    auto orbitSelection = std::make_shared<we::runtime::kindui::ToolButton>(
        we::runtime::kindui::Icons::SettingsName,
        std::string("Orbit Around Selection: ") + (settings.orbitAroundSelection ? "On" : "Off"),
        []() {
            auto& navStore = ViewportNavigationSettingsStore::Get();
            navStore.GetMutableSettings().orbitAroundSelection = !navStore.GetSettings().orbitAroundSelection;
            navStore.Save();
        },
        "Toggle orbit around selection");
    content->AddChild(orbitSelection);

    auto focusSelection = std::make_shared<we::runtime::kindui::ToolButton>(
        we::runtime::kindui::Icons::SettingsName,
        std::string("Focus On Selection (F): ") + (settings.focusOnSelection ? "On" : "Off"),
        []() {
            auto& navStore = ViewportNavigationSettingsStore::Get();
            navStore.GetMutableSettings().focusOnSelection = !navStore.GetSettings().focusOnSelection;
            navStore.Save();
        },
        "Toggle focus on selection");
    content->AddChild(focusSelection);

    return content;
}

} // namespace

std::shared_ptr<we::editor::ui::Panel> CreateViewportNavigationPreferencesPanel() {
    return we::editor::ui::PanelBuilder("Viewport Navigation")
        .TabIcon(we::runtime::kindui::Icons::SettingsName)
        .Content(BuildViewportNavigationPreferencesContent());
}

REGISTER_UI_PANEL(ViewportNavigation,
    WE_PANEL(ViewportNavigation).Title("Viewport Navigation").Icon("settings").Zone(we::editor::ui::DockZone::Floating).Hidden(),
    CreateViewportNavigationPreferencesPanel)

void ShowViewportNavigationPreferences() {
    EditorWorkspaceController::Get().FocusViewportNavigationPanel();
}

} // namespace we::programs::editor
