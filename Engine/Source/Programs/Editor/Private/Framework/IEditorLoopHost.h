// ==============================================================================
// WindEffects — Editor — IEditorLoopHost
// Host services the application-loop subsystems call into (owned by Editor).
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <memory>

#include "Platform/Types.h"

namespace we::runtime::renderer {
class Renderer;
struct CameraUniform;
}
namespace we::runtime::engine {
class EditorCamera;
}
namespace we::runtime::scene {
class Scene;
}
namespace we::runtime::kindui {
class Widget;
class EventSystem;
class OverlayHost;
class OverlayRenderer;
}

namespace we::programs::editor {

/**
 * Narrow host surface for loop subsystems. Keeps Editor ownership of
 * renderer/UI objects while the framework drives the frame.
 */
class IEditorLoopHost {
public:
    virtual ~IEditorLoopHost() = default;

    virtual we::platform::WindowId GetHostWindow() const = 0;
    virtual bool IsHostRunning() const = 0;
    virtual void RequestHostStop() = 0;

    virtual we::runtime::renderer::Renderer* GetHostRenderer() = 0;
    virtual we::runtime::engine::EditorCamera* GetHostCamera() = 0;
    virtual we::runtime::scene::Scene* GetHostScene() = 0;

    virtual std::shared_ptr<we::runtime::kindui::Widget> GetHostRootWidget() = 0;
    virtual std::shared_ptr<we::runtime::kindui::Widget> GetHostViewportWidget() = 0;
    virtual std::shared_ptr<we::runtime::kindui::EventSystem> GetHostUIEventSystem() = 0;
    virtual std::shared_ptr<we::runtime::kindui::OverlayHost> GetHostOverlayHost() = 0;
    virtual we::runtime::kindui::OverlayRenderer* GetHostOverlayRenderer() = 0;

    virtual we::platform::Int2& HostLastSampledMousePos() = 0;
    virtual uint64_t& HostLastSceneCameraHash() = 0;
    virtual bool& HostHasRenderedScene() = 0;
    virtual bool& HostForceSwapchainRecreate() = 0;
    virtual uint64_t& HostLatencyAuditFrameCounter() = 0;
    virtual bool HostFirstRunAgreementPending() const = 0;

    virtual void HostEnsureVisibleSwapchain() = 0;
    virtual bool HostSyncViewportFramebufferFromLayout() = 0;
    virtual void HostUpdateUiScaleFromWindow() = 0;
    virtual void HostTickSimulation(float dt) = 0;
    virtual void HostProcessLateInputMouse() = 0;
    virtual void HostMaybeShowFirstRunAgreement() = 0;
    virtual void HostReloadLayout() = 0;
    virtual bool HostIsRemoteApiEnabled() const = 0;
};

} // namespace we::programs::editor
