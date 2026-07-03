#include "Core/Environment.hpp"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#else
#include <cstdlib>
#endif

namespace we::core {

std::optional<std::string> GetEnvironmentVariable(const char* name) {
    if (name == nullptr || *name == '\0') {
        return std::nullopt;
    }

#if defined(_WIN32)
    char* buffer = nullptr;
    size_t length = 0;
    if (_dupenv_s(&buffer, &length, name) != 0 || buffer == nullptr) {
        free(buffer);
        return std::nullopt;
    }

    std::string value(buffer);
    free(buffer);
    return value;
#else
    if (const char* value = std::getenv(name)) {
        return std::string(value);
    }

    return std::nullopt;
#endif
}

} // namespace we::core
