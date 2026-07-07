#pragma once

#include <string_view>

namespace we::runtime::core {

// Canonical subsystem categories for structured logging.
namespace LogCategory {
    inline constexpr std::string_view General     = "General";
    inline constexpr std::string_view Renderer    = "Renderer";
    inline constexpr std::string_view Vulkan      = "Vulkan";
    inline constexpr std::string_view Shader      = "Shader";
    inline constexpr std::string_view Pipeline    = "Pipeline";
    inline constexpr std::string_view Resource      = "Resource";
    inline constexpr std::string_view Asset         = "Asset";
    inline constexpr std::string_view ECS           = "ECS";
    inline constexpr std::string_view Environment   = "Environment";
    inline constexpr std::string_view Editor        = "Editor";
    inline constexpr std::string_view Physics       = "Physics";
    inline constexpr std::string_view Audio         = "Audio";
    inline constexpr std::string_view Network       = "Network";
    inline constexpr std::string_view Plugin        = "Plugin";
    inline constexpr std::string_view Build         = "Build";
    inline constexpr std::string_view Startup       = "Startup";
    inline constexpr std::string_view Diagnostics   = "Diagnostics";
    inline constexpr std::string_view Crash         = "Crash";
}

} // namespace we::runtime::core

// Re-export categories for unqualified lookup from we::runtime::* child namespaces.
namespace we::runtime::LogCategory {
    using namespace core::LogCategory;
}

namespace we::LogCategory {
    using namespace runtime::core::LogCategory;
}
