// ==============================================================================
// WindEffects — Text — Errors
// Public API surface for the Text module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Text/Export.h"

#include <span>
#include <string>
#include <utility>

namespace we::runtime::text {

struct TextError {
    std::string message;
    std::string context;
};

template<typename T>
struct TextResult {
    bool ok = false;
    T value{};
    TextError error{};

    [[nodiscard]] static TextResult Success(T value)
    {
        TextResult result;
        result.ok = true;
        result.value = std::move(value);
        return result;
    }

    [[nodiscard]] static TextResult Failure(std::string message, std::string context = {})
    {
        TextResult result;
        result.ok = false;
        result.error.message = std::move(message);
        result.error.context = std::move(context);
        return result;
    }
};

template<>
struct TextResult<void> {
    bool ok = false;
    TextError error{};

    [[nodiscard]] static TextResult Success()
    {
        TextResult result;
        result.ok = true;
        return result;
    }

    [[nodiscard]] static TextResult Failure(std::string message, std::string context = {})
    {
        TextResult result;
        result.ok = false;
        result.error.message = std::move(message);
        result.error.context = std::move(context);
        return result;
    }
};

} // namespace we::runtime::text
