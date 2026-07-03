#pragma once

#include <optional>
#include <string>

namespace we::core {

/// Reads an environment variable. Returns nullopt when the variable is unset.
std::optional<std::string> GetEnvVar(const char* name);

} // namespace we::core
