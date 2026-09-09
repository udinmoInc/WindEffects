// ==============================================================================
// WindEffects — World — ITickSystem
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "World/Export.h"
#include "World/WorldTypes.h"

#include <functional>
#include <memory>
#include <string_view>

namespace we::runtime::world {

class IWorldContext;

using TickFunction = std::function<void(IWorldContext& context, float deltaSeconds)>;

struct WORLD_API TickRegistration {
    char name[64]{};
    TickGroup group = TickGroup::DuringUpdate;
    float priority = 0.f;
    bool canTickAsync = false;
    bool tickEvenWhenPaused = false;
};

/// Deterministic tick scheduler with fixed update and BeginPlay/EndPlay flush points.
class WORLD_API ITickSystem {
public:
    virtual ~ITickSystem() = default;

    [[nodiscard]] virtual std::uint64_t Register(const TickRegistration& reg, TickFunction fn) = 0;
    virtual bool Unregister(std::uint64_t tickId) = 0;

    virtual void SetFixedDeltaSeconds(float seconds) = 0;
    [[nodiscard]] virtual float FixedDeltaSeconds() const noexcept = 0;
    [[nodiscard]] virtual double Accumulator() const noexcept = 0;

    virtual void Tick(IWorldContext& context, const WorldTickParams& params) = 0;
    virtual void FlushBeginPlay(IWorldContext& context) = 0;
    virtual void FlushEndPlay(IWorldContext& context) = 0;
};

} // namespace we::runtime::world
