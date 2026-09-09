// ==============================================================================
// WindEffects — Text — Types
// Public API surface for the Text module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include <cstdint>

namespace we::runtime::text {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct RectUv {
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
};

struct Bounds {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

struct ColorRGBA {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

using Codepoint = char32_t;
using FontHandle = uint32_t;
using AtlasPageHandle = uint32_t;

constexpr FontHandle kInvalidFontHandle = 0;
constexpr AtlasPageHandle kInvalidAtlasPageHandle = 0;

} // namespace we::runtime::text
