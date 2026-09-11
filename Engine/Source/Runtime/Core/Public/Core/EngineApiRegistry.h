// ==============================================================================
// WindEffects — Core — EngineApiRegistry
// Lightweight command registry for tools, automation, and AI assistants.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace we::runtime::core {

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

/**
 * Process-wide API registry.
 * Handlers take/return JSON text (nlohmann when available) so clients stay language-agnostic.
 */
class CORE_API EngineApiRegistry {
public:
    using Handler = std::function<std::string(const std::string& argsJson)>;

    static EngineApiRegistry& Get();

    void Register(std::string_view command, Handler handler);
    void Unregister(std::string_view command);
    void Clear();

    [[nodiscard]] bool Has(std::string_view command) const;
    [[nodiscard]] std::vector<std::string> ListCommands() const;

    /// Invoke a registered command. Returns a protocol response JSON object string.
    [[nodiscard]] std::string Invoke(std::string_view command, const std::string& argsJson = "{}") const;

    /// Runtime key/value overlay for config injection (non-persistent).
    void SetConfigValue(std::string_view key, std::string_view value);
    [[nodiscard]] std::string GetConfigValue(std::string_view key, std::string_view fallback = {}) const;
    [[nodiscard]] std::unordered_map<std::string, std::string> GetAllConfigValues() const;

private:
    EngineApiRegistry() = default;

    mutable std::mutex m_Mutex;
    std::unordered_map<std::string, Handler> m_Handlers;
    std::unordered_map<std::string, std::string> m_ConfigOverlay;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

} // namespace we::runtime::core
