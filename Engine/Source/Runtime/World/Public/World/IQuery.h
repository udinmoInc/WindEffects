#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace we::runtime::world {

struct WORLD_API ActorQueryFilter {
    TagId requiredTags{};
    TagId excludedTags{};
    LayerId layer{};
    LevelHandle level{};
    bool requireBegunPlay = false;
};

using ActorPredicate = std::function<bool(ActorHandle)>;

class WORLD_API IQuerySystem {
public:
    virtual ~IQuerySystem() = default;

    [[nodiscard]] virtual std::vector<ActorHandle> Query(const ActorQueryFilter& filter) const = 0;
    [[nodiscard]] virtual std::vector<ActorHandle> QueryByName(std::string_view name) const = 0;
    [[nodiscard]] virtual std::vector<ActorHandle> QueryWhere(const ActorPredicate& predicate) const = 0;
    [[nodiscard]] virtual ActorHandle FindFirst(const ActorQueryFilter& filter) const = 0;
};

struct WORLD_API SpatialQueryParams {
    SpatialQueryShape shape = SpatialQueryShape::Sphere;
    Vec3f center{};
    Vec3f extents{1.f, 1.f, 1.f};
    float radius = 1.f;
    ActorQueryFilter filter{};
    std::uint32_t maxResults = 256;
};

class WORLD_API ISpatialQuery {
public:
    virtual ~ISpatialQuery() = default;

    virtual void Rebuild() = 0;
    [[nodiscard]] virtual std::vector<ActorHandle> Query(const SpatialQueryParams& params) const = 0;
    [[nodiscard]] virtual std::vector<ActorHandle> OverlapSphere(const Sphere3f& sphere, const ActorQueryFilter& filter = {}) const = 0;
    [[nodiscard]] virtual std::vector<ActorHandle> OverlapBox(const Aabb3f& box, const ActorQueryFilter& filter = {}) const = 0;
};

} // namespace we::runtime::world
