// ==============================================================================
// WindEffects — World — IWorldRuntime
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"
#include "World/IWorldManager.h"
#include "World/WorldTypes.h"

#include <functional>
#include <memory>

namespace we::runtime::assetruntime {
class IAssetManager;
}

namespace we::runtime::reflection {
class ITypeRegistry;
}

namespace we::runtime::serialization {
class ISerializer;
}

namespace we::runtime::world {

class IPhysicsWorldHook;
class IAudioWorldHook;
class INetworkWorldHook;
class IEditorWorldHook;
class IRenderWorldHook;

/// Explicit dependency injection for World Runtime construction (no service location).
struct WORLD_API WorldRuntimeDependencies {
    reflection::ITypeRegistry* typeRegistry = nullptr;
    serialization::ISerializer* serializer = nullptr;
    assetruntime::IAssetManager* assetManager = nullptr;
    std::shared_ptr<IPhysicsWorldHook> physicsHook;
    std::shared_ptr<IAudioWorldHook> audioHook;
    std::shared_ptr<INetworkWorldHook> networkHook;
    std::shared_ptr<IEditorWorldHook> editorHook;
    std::shared_ptr<IRenderWorldHook> renderHook;
    std::function<void(std::string_view)> onLog;
    bool registerReflectionTypes = true;
    bool installDefaultSubsystemFactories = true;
};

/// Top-level World Runtime facade — owns the WorldManager and extension wiring.
class WORLD_API IWorldRuntime {
public:
    virtual ~IWorldRuntime() = default;

    [[nodiscard]] virtual IWorldManager& Manager() noexcept = 0;
    [[nodiscard]] virtual const IWorldManager& Manager() const noexcept = 0;

    virtual void Tick(const WorldTickParams& params) = 0;
    virtual void Shutdown() = 0;
};

[[nodiscard]] WORLD_API std::unique_ptr<IWorldRuntime> CreateWorldRuntime(
    WorldRuntimeDependencies deps = {});

} // namespace we::runtime::world
