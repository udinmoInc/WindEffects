#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

namespace we::runtime::world {

class IWorld;
class IWorldContext;
class IActor;

/// Optional integration hooks — World Runtime never hardcodes physics/audio/net/editor/renderer.

class WORLD_API IPhysicsWorldHook {
public:
    virtual ~IPhysicsWorldHook() = default;
    virtual void OnWorldCreated(IWorld& world) = 0;
    virtual void OnWorldDestroyed(IWorld& world) = 0;
    virtual void OnActorSpawned(IWorld& world, IActor& actor) = 0;
    virtual void OnActorDestroyed(IWorld& world, IActor& actor) = 0;
    virtual void OnPrePhysicsTick(IWorldContext& context, float dt) = 0;
    virtual void OnPostPhysicsTick(IWorldContext& context, float dt) = 0;
};

class WORLD_API IAudioWorldHook {
public:
    virtual ~IAudioWorldHook() = default;
    virtual void OnWorldCreated(IWorld& world) = 0;
    virtual void OnWorldDestroyed(IWorld& world) = 0;
    virtual void OnListenerUpdated(IWorldContext& context, const Vec3f& position) = 0;
};

class WORLD_API INetworkWorldHook {
public:
    virtual ~INetworkWorldHook() = default;
    virtual void OnWorldCreated(IWorld& world) = 0;
    virtual void OnWorldDestroyed(IWorld& world) = 0;
    virtual void OnActorSpawned(IWorld& world, IActor& actor) = 0;
    virtual void OnActorDestroyed(IWorld& world, IActor& actor) = 0;
    virtual void OnNetTick(IWorldContext& context, float dt) = 0;
};

class WORLD_API IEditorWorldHook {
public:
    virtual ~IEditorWorldHook() = default;
    virtual void OnWorldCreated(IWorld& world) = 0;
    virtual void OnWorldDestroyed(IWorld& world) = 0;
    virtual void OnSelectionChanged(IWorld& world, ActorHandle selected) = 0;
    virtual void OnUndoRedoBoundary(IWorld& world) = 0;
};

class WORLD_API IRenderWorldHook {
public:
    virtual ~IRenderWorldHook() = default;
    virtual void OnWorldCreated(IWorld& world) = 0;
    virtual void OnWorldDestroyed(IWorld& world) = 0;
    virtual void OnExtractFrame(IWorldContext& context) = 0;
};

} // namespace we::runtime::world
