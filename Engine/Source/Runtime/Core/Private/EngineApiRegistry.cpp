// ==============================================================================
// WindEffects — Core — EngineApiRegistry
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/EngineApiRegistry.h"

#include <sstream>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::core {

EngineApiRegistry& EngineApiRegistry::Get() {
    static EngineApiRegistry instance;
    return instance;
}

void EngineApiRegistry::Register(std::string_view command, Handler handler) {
    if (command.empty() || !handler) {
        return;
    }
    std::lock_guard lock(m_Mutex);
    m_Handlers[std::string(command)] = std::move(handler);
}

void EngineApiRegistry::Unregister(std::string_view command) {
    std::lock_guard lock(m_Mutex);
    m_Handlers.erase(std::string(command));
}

void EngineApiRegistry::Clear() {
    std::lock_guard lock(m_Mutex);
    m_Handlers.clear();
}

bool EngineApiRegistry::Has(std::string_view command) const {
    std::lock_guard lock(m_Mutex);
    return m_Handlers.find(std::string(command)) != m_Handlers.end();
}

std::vector<std::string> EngineApiRegistry::ListCommands() const {
    std::lock_guard lock(m_Mutex);
    std::vector<std::string> out;
    out.reserve(m_Handlers.size());
    for (const auto& [name, _] : m_Handlers) {
        out.push_back(name);
    }
    return out;
}

std::string EngineApiRegistry::Invoke(std::string_view command, const std::string& argsJson) const {
    Handler handler;
    {
        std::lock_guard lock(m_Mutex);
        auto it = m_Handlers.find(std::string(command));
        if (it == m_Handlers.end()) {
#if WE_HAS_NLOHMANN_JSON
            nlohmann::json err = {
                {"ok", false},
                {"error", std::string("Unknown command: ") + std::string(command)},
                {"result", nlohmann::json::object()}
            };
            return err.dump();
#else
            return std::string("{\"ok\":false,\"error\":\"Unknown command: ") + std::string(command) +
                   "\",\"result\":{}}";
#endif
        }
        handler = it->second;
    }

    try {
        const std::string result = handler(argsJson.empty() ? "{}" : argsJson);
#if WE_HAS_NLOHMANN_JSON
        nlohmann::json wrapped;
        wrapped["ok"] = true;
        wrapped["error"] = nullptr;
        try {
            wrapped["result"] = nlohmann::json::parse(result.empty() ? "{}" : result);
        } catch (...) {
            wrapped["result"] = result;
        }
        return wrapped.dump();
#else
        return std::string("{\"ok\":true,\"error\":null,\"result\":") +
               (result.empty() ? std::string("{}") : result) + "}";
#endif
    } catch (const std::exception& ex) {
#if WE_HAS_NLOHMANN_JSON
        nlohmann::json err = {
            {"ok", false},
            {"error", ex.what()},
            {"result", nlohmann::json::object()}
        };
        return err.dump();
#else
        return std::string("{\"ok\":false,\"error\":\"") + ex.what() + "\",\"result\":{}}";
#endif
    }
}

void EngineApiRegistry::SetConfigValue(std::string_view key, std::string_view value) {
    std::lock_guard lock(m_Mutex);
    m_ConfigOverlay[std::string(key)] = std::string(value);
}

std::string EngineApiRegistry::GetConfigValue(std::string_view key, std::string_view fallback) const {
    std::lock_guard lock(m_Mutex);
    auto it = m_ConfigOverlay.find(std::string(key));
    if (it == m_ConfigOverlay.end()) {
        return std::string(fallback);
    }
    return it->second;
}

std::unordered_map<std::string, std::string> EngineApiRegistry::GetAllConfigValues() const {
    std::lock_guard lock(m_Mutex);
    return m_ConfigOverlay;
}

} // namespace we::runtime::core
