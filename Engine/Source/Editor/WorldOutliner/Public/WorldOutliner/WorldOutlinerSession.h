// ==============================================================================
// WindEffects — WorldOutliner — WorldOutlinerSession
// Public API surface for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "WorldOutliner/Export.h"
#include "WorldOutliner/IWorldOutliner.h"

#include <memory>

namespace we::editor::outliner {

/// Editor-session binding for panel factories that cannot take constructor DI.
class WORLDOUTLINER_API WorldOutlinerSession {
public:
    static void Install(std::shared_ptr<IWorldOutlinerRuntime> runtime);
    static void Clear() noexcept;

    [[nodiscard]] static IWorldOutlinerRuntime* Runtime() noexcept;
    [[nodiscard]] static IWorldOutliner* Outliner() noexcept;
    [[nodiscard]] static std::shared_ptr<IWorldOutlinerRuntime> RuntimeShared() noexcept;
    [[nodiscard]] static bool IsInstalled() noexcept;
};

} // namespace we::editor::outliner
