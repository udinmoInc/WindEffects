#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <memory>
#include <span>
#include <string_view>

namespace we::runtime::scene {
class Scene;
}

namespace we::runtime::world {

class IWorld;
class IWorldRegistry;

struct WORLD_API WorldCreateInfo {
    WorldDescriptor descriptor{};
    std::shared_ptr<scene::Scene> existingScene; // optional; otherwise a Scene is created
    bool createPersistentLevel = true;
    bool registerDefaultSubsystems = true;
    bool isEditor = false;
};

/// Multi-world orchestration — create/destroy/tick many worlds (PIE, servers, menus, etc.).
class WORLD_API IWorldManager {
public:
    virtual ~IWorldManager() = default;

    [[nodiscard]] virtual IWorldRegistry& Registry() noexcept = 0;
    [[nodiscard]] virtual const IWorldRegistry& Registry() const noexcept = 0;

    [[nodiscard]] virtual WorldHandle CreateWorld(const WorldCreateInfo& info) = 0;
    [[nodiscard]] virtual bool DestroyWorld(WorldHandle handle) = 0;

    [[nodiscard]] virtual IWorld* FindWorld(WorldHandle handle) noexcept = 0;
    [[nodiscard]] virtual const IWorld* FindWorld(WorldHandle handle) const noexcept = 0;
    [[nodiscard]] virtual IWorld* FindWorldByGuid(const WorldGuid& guid) noexcept = 0;
    [[nodiscard]] virtual IWorld* FindWorldByName(std::string_view name) noexcept = 0;

    [[nodiscard]] virtual std::span<const WorldHandle> Worlds() const noexcept = 0;
    [[nodiscard]] virtual WorldHandle ActiveWorld() const noexcept = 0;
    virtual void SetActiveWorld(WorldHandle handle) = 0;

    virtual void TickAll(const WorldTickParams& params) = 0;
    virtual void TickWorld(WorldHandle handle, const WorldTickParams& params) = 0;

    virtual void Shutdown() = 0;
};

} // namespace we::runtime::world
