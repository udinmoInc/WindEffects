// ==============================================================================
// WindEffects — WeLauncher — RenameDialogView
// UI widget used by the WeLauncher module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "KindUI/Declarative/Element.h"

#include <functional>
#include <string>

namespace we::programs::welauncher {

struct RenameDialogState {
    std::string name;
    std::function<void()> onCancel;
    std::function<void()> onRename;
    std::function<void(const std::string&)> onNameChanged;
};

[[nodiscard]] we::runtime::kindui::Element BuildRenameDialogView(const RenameDialogState& state);

} // namespace we::programs::welauncher
