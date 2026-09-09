// ==============================================================================
// WindEffects — AssetProcessors — BuiltinProcessors
// Internal implementation for the AssetProcessors module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "AssetProcessors/ProcessorRegistry.h"

namespace we::runtime::assetprocessors {

void RegisterBuiltinProcessors(ProcessorRegistry& registry);

} // namespace we::runtime::assetprocessors
