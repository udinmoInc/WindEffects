#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

namespace we::runtime::world {

/// Transform hierarchy: local ↔ world matrices. Syncs with ECS TransformComponent when bound.
class WORLD_API ITransformHierarchy {
public:
    virtual ~ITransformHierarchy() = default;

    virtual void SetLocalPosition(ActorHandle actor, const Vec3f& position) = 0;
    virtual void SetLocalRotationEuler(ActorHandle actor, const Vec3f& eulerDegrees) = 0;
    virtual void SetLocalScale(ActorHandle actor, const Vec3f& scale) = 0;

    [[nodiscard]] virtual Vec3f GetLocalPosition(ActorHandle actor) const = 0;
    [[nodiscard]] virtual Vec3f GetLocalRotationEuler(ActorHandle actor) const = 0;
    [[nodiscard]] virtual Vec3f GetLocalScale(ActorHandle actor) const = 0;
    [[nodiscard]] virtual Vec3f GetWorldPosition(ActorHandle actor) const = 0;

    virtual void MarkDirty(ActorHandle actor) = 0;
    virtual void UpdateWorldTransforms() = 0;
    virtual void SyncToEcs() = 0;
    virtual void SyncFromEcs() = 0;
};

} // namespace we::runtime::world
