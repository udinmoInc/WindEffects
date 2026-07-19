#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <memory>
#include <span>
#include <string_view>

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::ecs {
class Registry;
class World;
}

namespace we::runtime::world {

/// Streaming / persistence unit within a world. Owns or shares a Scene for ECS storage.
class WORLD_API ILevel {
public:
    virtual ~ILevel() = default;

    [[nodiscard]] virtual LevelHandle Handle() const noexcept = 0;
    [[nodiscard]] virtual const WorldGuid& Guid() const noexcept = 0;
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual LevelState State() const noexcept = 0;
    [[nodiscard]] virtual const LevelDescriptor& Descriptor() const noexcept = 0;
    [[nodiscard]] virtual bool IsPersistent() const noexcept = 0;
    [[nodiscard]] virtual WorldHandle OwningWorld() const noexcept = 0;

    [[nodiscard]] virtual scene::Scene& Scene() noexcept = 0;
    [[nodiscard]] virtual const scene::Scene& Scene() const noexcept = 0;
    [[nodiscard]] virtual ecs::Registry& Registry() noexcept = 0;
    [[nodiscard]] virtual ecs::World& EcsWorld() noexcept = 0;

    [[nodiscard]] virtual Aabb3f Bounds() const noexcept = 0;
    virtual void SetBounds(const Aabb3f& bounds) = 0;

    [[nodiscard]] virtual std::span<const ActorHandle> Actors() const noexcept = 0;

    virtual void SetVisible(bool visible) = 0;
    [[nodiscard]] virtual bool IsVisible() const noexcept = 0;

    virtual bool Load() = 0;
    virtual bool Unload() = 0;
    virtual void Tick(float deltaSeconds) = 0;
};

} // namespace we::runtime::world
