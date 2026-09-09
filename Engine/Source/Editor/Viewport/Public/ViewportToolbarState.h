// ==============================================================================
// WindEffects — Viewport — ViewportToolbarState
// Public API surface for the Viewport module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Viewport/Export.h"

#include <memory>

namespace we::editor::toolbar {
class ToolButton;
}

namespace we::runtime::engine {
class EditorCamera;
}

namespace we::programs::editor {

VIEWPORT_API void BindViewportCamera(const std::shared_ptr<we::runtime::engine::EditorCamera>& camera);

VIEWPORT_API void SetViewportCameraSpeedIndicator(const std::shared_ptr<::we::editor::toolbar::ToolButton>& indicator);
VIEWPORT_API void UpdateViewportCameraSpeedIndicator();
VIEWPORT_API void StepViewportCameraSpeed(int direction);
VIEWPORT_API void AdjustViewportCameraSpeedFromWheel(float wheelDeltaY);

VIEWPORT_API void ShowViewportCameraSpeedPopup();

VIEWPORT_API void ApplyLoadedViewportNavigationSettings();

} // namespace we::programs::editor
