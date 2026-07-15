#pragma once

#include "Core/Logger.h"
#include "Core/DiagnosticMacros.h"
#include <string>
#include <cstdlib>

namespace we::runtime::renderer {

// Validates initialization. If cond is false, logs a fatal error and terminates.
#define WE_VALIDATE_INIT(cond, subsystem, reason) \
    do { \
        if (!(cond)) { \
            std::string msg = std::string("Initialization failed in subsystem '") + (subsystem) + "': " + (reason) + \
                              "\nFile: " + __FILE__ + "\nFunction: " + __FUNCTION__; \
            WE_LOG_CRITICAL("RendererInit", msg); \
            std::abort(); \
        } \
    } while(0)

// Validates a render pass requirement.
#define WE_VALIDATE_RENDER(cond, passName, reason) \
    do { \
        if (!(cond)) { \
            std::string msg = std::string("Validation failed during render pass '") + (passName) + "': " + (reason) + \
                              "\nFile: " + __FILE__ + "\nFunction: " + __FUNCTION__; \
            WE_LOG_CRITICAL("RendererRender", msg); \
            std::abort(); \
        } \
    } while(0)

} // namespace we::runtime::renderer
