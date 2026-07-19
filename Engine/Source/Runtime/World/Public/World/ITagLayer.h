#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <span>
#include <string_view>

namespace we::runtime::world {

class WORLD_API ITagSystem {
public:
    virtual ~ITagSystem() = default;

    [[nodiscard]] virtual TagId RegisterTag(std::string_view name) = 0;
    [[nodiscard]] virtual TagId FindTag(std::string_view name) const noexcept = 0;
    [[nodiscard]] virtual std::string_view TagName(TagId id) const noexcept = 0;

    virtual void AddTag(ActorHandle actor, TagId tag) = 0;
    virtual void RemoveTag(ActorHandle actor, TagId tag) = 0;
    [[nodiscard]] virtual bool HasTag(ActorHandle actor, TagId tag) const noexcept = 0;
    [[nodiscard]] virtual TagId GetTags(ActorHandle actor) const noexcept = 0;

    [[nodiscard]] virtual std::span<const ActorHandle> QueryTag(TagId tag) const = 0;
};

class WORLD_API ILayerSystem {
public:
    virtual ~ILayerSystem() = default;

    [[nodiscard]] virtual LayerId RegisterLayer(std::string_view name) = 0;
    [[nodiscard]] virtual LayerId FindLayer(std::string_view name) const noexcept = 0;
    [[nodiscard]] virtual std::string_view LayerName(LayerId id) const noexcept = 0;

    virtual void SetLayer(ActorHandle actor, LayerId layer) = 0;
    [[nodiscard]] virtual LayerId GetLayer(ActorHandle actor) const noexcept = 0;
    [[nodiscard]] virtual std::span<const ActorHandle> QueryLayer(LayerId layer) const = 0;

    virtual void SetLayerVisible(LayerId layer, bool visible) = 0;
    [[nodiscard]] virtual bool IsLayerVisible(LayerId layer) const noexcept = 0;
};

} // namespace we::runtime::world
