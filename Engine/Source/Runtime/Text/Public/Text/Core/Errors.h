#pragma once

#include "Text/Export.h"

#include <string>
#include <utility>

namespace we::runtime::text {

struct TEXT_API TextError {
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

} // namespace we::runtime::text
