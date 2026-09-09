// ==============================================================================
// WindEffects — KindUI — UiDebugImageWriter
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace we::runtime::kindui {

// Writes uncompressed 32-bit BMP (BGRA) for atlas / icon pipeline debugging.
bool SaveBmpRgba(const std::string& path, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height);

} // namespace we::runtime::kindui
