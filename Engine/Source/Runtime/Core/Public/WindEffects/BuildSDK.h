#pragma once

// WindEffects Build SDK — invoke IgniteBT from any C++ program.

#include "WindEffects/Platform.h"
#include "Core/IgniteBTInvoker.h"

#include <string>
#include <vector>

namespace we::build {

struct BuildRequest {
    std::vector<std::string> arguments;
};

inline we::core::IgniteBTInvokeResult CompileDebug() {
    return we::core::InvokeIgniteBT({"build", "--config", "Debug"});
}

inline we::core::IgniteBTInvokeResult CompileDevelopment() {
    return we::core::InvokeIgniteBT({"build", "--config", "Development"});
}

inline we::core::IgniteBTInvokeResult Run(const std::vector<std::string>& arguments) {
    return we::core::InvokeIgniteBT(arguments);
}

} // namespace we::build
