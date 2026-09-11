// ==============================================================================
// WindEffects — Core — EngineRemoteProtocol
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Core/EngineRemoteProtocol.h"

#include <cstring>
#include <sstream>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::runtime::core {
namespace {

uint32_t FastHash(std::string_view text) {
    uint32_t hash = 2166136261u;
    for (char c : text) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

} // namespace

std::string EngineRemoteProtocol::SerializeRequest(const EngineRemoteRequest& request) {
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json root = {
        {"id", request.id},
        {"command", request.command}
    };
    try {
        root["args"] = nlohmann::json::parse(request.argsJson.empty() ? "{}" : request.argsJson);
    } catch (...) {
        root["args"] = nlohmann::json::object();
    }
    return root.dump();
#else
    return std::string("{\"id\":\"") + request.id + "\",\"command\":\"" + request.command +
           "\",\"args\":" + (request.argsJson.empty() ? "{}" : request.argsJson) + "}";
#endif
}

bool EngineRemoteProtocol::TryParseRequest(std::string_view json, EngineRemoteRequest& out) {
#if WE_HAS_NLOHMANN_JSON
    try {
        const auto root = nlohmann::json::parse(json);
        out.id = root.value("id", "");
        out.command = root.value("command", "");
        if (root.contains("args")) {
            out.argsJson = root["args"].dump();
        } else {
            out.argsJson = "{}";
        }
        return !out.command.empty();
    } catch (...) {
        return false;
    }
#else
    (void)json;
    (void)out;
    return false;
#endif
}

std::string EngineRemoteProtocol::SerializeResponse(const EngineRemoteResponse& response) {
#if WE_HAS_NLOHMANN_JSON
    nlohmann::json root = {
        {"id", response.id},
        {"ok", response.ok},
        {"error", response.error.empty() ? nlohmann::json(nullptr) : nlohmann::json(response.error)}
    };
    try {
        root["result"] = nlohmann::json::parse(response.resultJson.empty() ? "{}" : response.resultJson);
    } catch (...) {
        root["result"] = response.resultJson;
    }
    return root.dump();
#else
    return std::string("{\"id\":\"") + response.id + "\",\"ok\":" + (response.ok ? "true" : "false") +
           ",\"result\":" + (response.resultJson.empty() ? "{}" : response.resultJson) +
           ",\"error\":" + (response.error.empty() ? "null" : ("\"" + response.error + "\"")) + "}";
#endif
}

bool EngineRemoteProtocol::TryParseResponse(std::string_view json, EngineRemoteResponse& out) {
#if WE_HAS_NLOHMANN_JSON
    try {
        const auto root = nlohmann::json::parse(json);
        out.id = root.value("id", "");
        out.ok = root.value("ok", false);
        out.error = root.contains("error") && !root["error"].is_null() ? root["error"].get<std::string>() : "";
        out.resultJson = root.contains("result") ? root["result"].dump() : "{}";
        return true;
    } catch (...) {
        return false;
    }
#else
    (void)json;
    (void)out;
    return false;
#endif
}

std::vector<uint8_t> EngineRemoteProtocol::EncodeFrame(std::string_view payload) {
    std::vector<uint8_t> out(4 + payload.size());
    const uint32_t len = static_cast<uint32_t>(payload.size());
    out[0] = static_cast<uint8_t>(len & 0xff);
    out[1] = static_cast<uint8_t>((len >> 8) & 0xff);
    out[2] = static_cast<uint8_t>((len >> 16) & 0xff);
    out[3] = static_cast<uint8_t>((len >> 24) & 0xff);
    std::memcpy(out.data() + 4, payload.data(), payload.size());
    return out;
}

bool EngineRemoteProtocol::TryDecodeFrame(
    const std::vector<uint8_t>& buffer, size_t& consumed, std::string& payload) {
    consumed = 0;
    if (buffer.size() < 4) {
        return false;
    }
    const uint32_t len = static_cast<uint32_t>(buffer[0])
        | (static_cast<uint32_t>(buffer[1]) << 8)
        | (static_cast<uint32_t>(buffer[2]) << 16)
        | (static_cast<uint32_t>(buffer[3]) << 24);
    if (buffer.size() < 4ull + len) {
        return false;
    }
    payload.assign(reinterpret_cast<const char*>(buffer.data() + 4), len);
    consumed = 4ull + len;
    return true;
}

std::filesystem::path EngineRemoteProtocol::ApiRoot(const std::filesystem::path& projectOrEngineRoot) {
    return projectOrEngineRoot / "Build" / "Temp" / "editor-api";
}

std::filesystem::path EngineRemoteProtocol::InboxDirectory(const std::filesystem::path& root) {
    return ApiRoot(root) / "inbox";
}

std::filesystem::path EngineRemoteProtocol::OutboxDirectory(const std::filesystem::path& root) {
    return ApiRoot(root) / "outbox";
}

std::filesystem::path EngineRemoteProtocol::EndpointFile(const std::filesystem::path& root) {
    return ApiRoot(root) / "endpoint";
}

std::filesystem::path EngineRemoteProtocol::PidFile(const std::filesystem::path& root) {
    return ApiRoot(root) / "pid";
}

std::string EngineRemoteProtocol::PipeName(const std::filesystem::path& root) {
    std::error_code ec;
    auto abs = std::filesystem::absolute(root, ec);
    if (ec) {
        abs = root;
    }
    std::string key = abs.lexically_normal().string();
    for (char& c : key) {
        if (c >= 'A' && c <= 'Z') {
            c = static_cast<char>(c - 'A' + 'a');
        }
    }
    std::ostringstream name;
    name << "we-editor-api-" << std::hex << FastHash(key);
    return name.str();
}

} // namespace we::runtime::core
