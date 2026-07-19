#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <cstdint>
#include <span>
#include <string_view>

namespace we::runtime::world {

/// Gameplay identity over an ECS entity. World Runtime owns actor lifetime policy;
/// ECS owns component storage for the bound entity.
class WORLD_API IActor {
public:
    virtual ~IActor() = default;

    [[nodiscard]] virtual ActorHandle Handle() const noexcept = 0;
    [[nodiscard]] virtual const WorldGuid& Guid() const noexcept = 0;
    [[nodiscard]] virtual std::string_view Name() const noexcept = 0;
    [[nodiscard]] virtual ActorLifecycle Lifecycle() const noexcept = 0;
    [[nodiscard]] virtual WorldHandle OwningWorld() const noexcept = 0;
    [[nodiscard]] virtual LevelHandle OwningLevel() const noexcept = 0;
    [[nodiscard]] virtual LayerId Layer() const noexcept = 0;
    [[nodiscard]] virtual TagId Tags() const noexcept = 0;

    /// Bound ECS entity id (0 if unbound).
    [[nodiscard]] virtual std::uint64_t EntityId() const noexcept = 0;
    [[nodiscard]] virtual bool IsValid() const noexcept = 0;
    [[nodiscard]] virtual bool HasBegunPlay() const noexcept = 0;

    virtual void SetName(std::string_view name) = 0;
    virtual void SetLayer(LayerId layer) = 0;
    virtual void SetTags(TagId tags) = 0;

    virtual void BeginPlay() = 0;
    virtual void EndPlay(EndPlayReason reason) = 0;
    virtual void Tick(float deltaSeconds) = 0;
};

class WORLD_API IActorRegistry {
public:
    virtual ~IActorRegistry() = default;

    [[nodiscard]] virtual ActorHandle Spawn(const ActorSpawnParams& params, LevelHandle level = {}) = 0;
    [[nodiscard]] virtual bool Destroy(ActorHandle actor, EndPlayReason reason = EndPlayReason::Destroyed) = 0;

    [[nodiscard]] virtual IActor* TryGet(ActorHandle handle) noexcept = 0;
    [[nodiscard]] virtual const IActor* TryGet(ActorHandle handle) const noexcept = 0;
    [[nodiscard]] virtual IActor* TryGetByGuid(const WorldGuid& guid) noexcept = 0;
    [[nodiscard]] virtual IActor* TryGetByEntityId(std::uint64_t entityId) noexcept = 0;
    [[nodiscard]] virtual IActor* TryGetByName(std::string_view name) noexcept = 0;

    [[nodiscard]] virtual std::span<const ActorHandle> All() const noexcept = 0;
    [[nodiscard]] virtual std::size_t Count() const noexcept = 0;

    virtual void FlushPendingBeginPlay() = 0;
    virtual void FlushPendingEndPlay() = 0;
};

} // namespace we::runtime::world
