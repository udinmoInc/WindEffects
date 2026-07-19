#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <cstdint>
#include <string_view>

namespace we::runtime::world {

struct WORLD_API PrefabInstanceParams {
    WorldGuid prefabGuid{};
    ActorSpawnParams spawn{};
    LevelHandle level{};
};

/// Prefab instancing foundation — delegates ECS entity cloning to ECS Prefab / SceneSerializer.
class WORLD_API IPrefabInstancer {
public:
    virtual ~IPrefabInstancer() = default;

    [[nodiscard]] virtual ActorHandle Instantiate(const PrefabInstanceParams& params) = 0;
    [[nodiscard]] virtual bool RegisterPrefabSource(const WorldGuid& prefabGuid, std::string_view assetPath) = 0;
    [[nodiscard]] virtual bool UnregisterPrefabSource(const WorldGuid& prefabGuid) = 0;
};

/// Large-world origin rebasing hooks (camera/player shift → rebase world origin).
class WORLD_API IOriginRebasing {
public:
    virtual ~IOriginRebasing() = default;

    virtual void SetEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsEnabled() const noexcept = 0;

    [[nodiscard]] virtual Vec3f CurrentOrigin() const noexcept = 0;
    virtual void RequestRebase(const Vec3f& newOrigin) = 0;
    virtual void ApplyPendingRebase() = 0;

    using RebaseCallback = void (*)(const Vec3f& oldOrigin, const Vec3f& newOrigin, void* userData);
    virtual void SetRebaseCallback(RebaseCallback callback, void* userData) = 0;
};

} // namespace we::runtime::world
