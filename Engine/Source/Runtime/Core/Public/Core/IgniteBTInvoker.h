#pragma once

#include "Core/Export.h"

#include <string>
#include <vector>

namespace we::core {

struct IgniteBTInvokeResult {
    int exitCode = -1;
    bool launched = false;
    std::string errorMessage;
};

IgniteBTInvokeResult CORE_API InvokeIgniteBT(const std::vector<std::string>& args);
bool CORE_API TryResolveIgniteBTExecutable(std::string& outExecutablePath, std::string& outWorkingDirectory);

} // namespace we::core
