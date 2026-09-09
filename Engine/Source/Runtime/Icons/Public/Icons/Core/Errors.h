// ==============================================================================
// WindEffects — Icons — Errors
// Public API surface for the Icons module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Icons/Export.h"

#include <string>
#include <utility>

namespace we::runtime::icons {

struct IconError {
    std::string message;
    std::string context;
};

template<typename T>
struct IconResult {
    bool ok = false;
    T value{};
    IconError error{};

    [[nodiscard]] static IconResult Success(T value)
    {
        IconResult result;
        result.ok = true;
        result.value = std::move(value);
        return result;
    }

    [[nodiscard]] static IconResult Failure(std::string message, std::string context = {})
    {
        IconResult result;
        result.ok = false;
        result.error.message = std::move(message);
        result.error.context = std::move(context);
        return result;
    }
};

template<>
struct IconResult<void> {
    bool ok = false;
    IconError error{};

    [[nodiscard]] static IconResult Success()
    {
        IconResult result;
        result.ok = true;
        return result;
    }

    [[nodiscard]] static IconResult Failure(std::string message, std::string context = {})
    {
        IconResult result;
        result.ok = false;
        result.error.message = std::move(message);
        result.error.context = std::move(context);
        return result;
    }
};

} // namespace we::runtime::icons
