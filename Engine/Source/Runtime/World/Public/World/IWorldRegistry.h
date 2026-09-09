// ==============================================================================
// WindEffects — World — IWorldRegistry
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <span>
#include <string_view>

namespace we::runtime::world {

class IWorld;

/// GUID / name / handle lookup for worlds. Lock-free reads after seal where possible.
class WORLD_API IWorldRegistry {
public:
    virtual ~IWorldRegistry() = default;

    [[nodiscard]] virtual bool Contains(WorldHandle handle) const noexcept = 0;
    [[nodiscard]] virtual bool ContainsGuid(const WorldGuid& guid) const noexcept = 0;

    [[nodiscard]] virtual IWorld* TryGet(WorldHandle handle) noexcept = 0;
    [[nodiscard]] virtual const IWorld* TryGet(WorldHandle handle) const noexcept = 0;
    [[nodiscard]] virtual IWorld* TryGetByGuid(const WorldGuid& guid) noexcept = 0;
    [[nodiscard]] virtual IWorld* TryGetByName(std::string_view name) noexcept = 0;

    [[nodiscard]] virtual std::span<const WorldHandle> All() const noexcept = 0;
    [[nodiscard]] virtual std::size_t Count() const noexcept = 0;
};

} // namespace we::runtime::world
