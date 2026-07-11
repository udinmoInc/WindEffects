#pragma once

#include "Core/Logger.h"
#include "Core/LogCategory.h"

namespace we::runtime::core {

inline const char* StripPath(const char* path) {
    if (!path) return "";
    const char* slash = path;
    for (const char* c = path; *c; ++c) {
        if (*c == '/' || *c == '\\') slash = c + 1;
    }
    return slash;
}

} // namespace we::runtime::core

#define WE_LOG_(levelEnum, category, message) \
    ::we::Logger::Log(levelEnum, category, message, \
        ::we::runtime::core::StripPath(__FILE__), __LINE__, __FUNCTION__)

#define WE_LOG_TRACE(category, message)   WE_LOG_(::we::Logger::Level::Trace, category, message)
#define WE_LOG_DEBUG(category, message)   WE_LOG_(::we::Logger::Level::Debug, category, message)
#define WE_LOG_INFO(category, message)    WE_LOG_(::we::Logger::Level::Info, category, message)
#define WE_LOG_WARN(category, message)    WE_LOG_(::we::Logger::Level::Warning, category, message)
#define WE_LOG_ERROR(category, message)   WE_LOG_(::we::Logger::Level::Error, category, message)
#define WE_LOG_CRITICAL(category, message) WE_LOG_(::we::Logger::Level::Critical, category, message)

// Legacy aliases — route to Startup category. Prefer WE_LOG_* with we::LogCategory.
#define HE_INFO(msg)  WE_LOG_INFO(we::LogCategory::Startup.data(), msg)
#define HE_WARN(msg)  WE_LOG_WARN(we::LogCategory::Startup.data(), msg)
#define HE_ERROR(msg) WE_LOG_ERROR(we::LogCategory::Startup.data(), msg)
#define HE_DEBUG(msg) WE_LOG_DEBUG(we::LogCategory::Startup.data(), msg)

#define WE_CHECK(condition, category, message) \
    do { \
        if (!(condition)) { \
            WE_LOG_ERROR(category, std::string("Check failed: ") + message); \
        } \
    } while (0)

#define WE_CHECK_FATAL(condition, category, message) \
    do { \
        if (!(condition)) { \
            ::we::Logger::ReportError("Fatal Check Failed", std::string(category) + ": " + message, true); \
        } \
    } while (0)
