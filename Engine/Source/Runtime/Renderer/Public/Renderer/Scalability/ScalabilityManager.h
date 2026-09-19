// ==============================================================================
// WindEffects — Renderer — ScalabilityManager
// Public API surface for the Renderer module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#pragma warning(disable : 4251)

#include "Renderer/Export.h"
#include "Renderer/Scalability/QualityTypes.h"
#include "Renderer/Scalability/RenderingSettings.h"
#include "RHI/Capabilities.h"
#include "RHI/Types.h"

#include <unordered_map>

namespace we::runtime::renderer {

struct ScalabilityDiagnosticsSnapshot;

/// Owns profile/configuration resolution only.
///
/// Ownership:
///   IRHI            — API/hardware abstraction
///   RHI Backend     — backend implementation
///   RenderGraph     — pass scheduling / transient resources
///   Renderer        — frame orchestration; owns this manager
///   ScalabilityManager — profile load, validate, capability resolve, publish
///   ResolvedRenderingSettings — configuration data (no RHI objects)
///   Feature systems (future) — read published settings; never own profiles
///
/// Does not render, allocate GPU resources, build RenderGraph passes,
/// or know backend command APIs.
class RENDERER_API ScalabilityManager {
public:
    ScalabilityManager();
    ~ScalabilityManager();

    ScalabilityManager(const ScalabilityManager&) = delete;
    ScalabilityManager& operator=(const ScalabilityManager&) = delete;

    void Initialize();
    void Shutdown();

    /// Load Engine/Config/Runtime/Scalability/Profiles/*.ini when present.
    bool ReloadProfiles();

    void SetRHICapabilities(const we::rhi::RHICapabilities& caps);
    void SetRHIBackend(we::rhi::RHIBackend backend);

    /// Queues a new resolved configuration. Published at PublishFrameSettings().
    [[nodiscard]] ScalabilityUpdateFlags SetProfile(RenderingProfileId id);

    [[nodiscard]] RenderingProfileId GetActiveProfileId() const { return m_ActiveProfileId; }

    /// Frame-stable settings for rendering. Call PublishFrameSettings() at BeginFrame.
    [[nodiscard]] const ResolvedRenderingSettings& GetPublishedSettings() const {
        return m_Published;
    }

    /// Latest requested settings (may differ from published until frame boundary).
    [[nodiscard]] const ResolvedRenderingSettings& GetPendingSettings() const {
        return m_Pending;
    }

    /// Copy pending → published. Safe to call at frame start on the render thread.
    void PublishFrameSettings();

    [[nodiscard]] bool HasPendingPublish() const { return m_PendingDirty; }

    [[nodiscard]] ScalabilityDiagnosticsSnapshot CaptureDiagnostics() const;

    [[nodiscard]] static RenderingProfileDesc MakeBuiltinProfile(RenderingProfileId id);
    [[nodiscard]] static ScalabilityUpdateFlags DiffSettings(
        const ResolvedRenderingSettings& previous,
        const ResolvedRenderingSettings& next);

private:
    void ResolveActiveProfile();
    void QueueResolved(ResolvedRenderingSettings resolved);

    bool m_Initialized = false;
    RenderingProfileId m_ActiveProfileId = RenderingProfileId::HighEnd;
    we::rhi::RHICapabilities m_Capabilities{};
    we::rhi::RHIBackend m_Backend = we::rhi::RHIBackend::Null;
    ResolvedRenderingSettings m_Pending{};
    ResolvedRenderingSettings m_Published{};
    bool m_PendingDirty = false;
    std::unordered_map<RenderingProfileId, RenderingProfileDesc> m_Profiles;
};

} // namespace we::runtime::renderer
