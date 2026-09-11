// ==============================================================================
// WindEffects — Core — EngineRemoteProtocol
// Length-prefixed JSON protocol + endpoint paths for Editor remote control.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/Export.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::core {

struct EngineRemoteRequest {
    std::string id;
    std::string command;
    std::string argsJson = "{}";
};

struct EngineRemoteResponse {
    std::string id;
    bool ok = false;
    std::string resultJson = "{}";
    std::string error;
};

class CORE_API EngineRemoteProtocol {
public:
    [[nodiscard]] static std::string SerializeRequest(const EngineRemoteRequest& request);
    [[nodiscard]] static bool TryParseRequest(std::string_view json, EngineRemoteRequest& out);
    [[nodiscard]] static std::string SerializeResponse(const EngineRemoteResponse& response);
    [[nodiscard]] static bool TryParseResponse(std::string_view json, EngineRemoteResponse& out);

    /// Encode/decode length-prefixed UTF-8 frames (4-byte little-endian length).
    [[nodiscard]] static std::vector<uint8_t> EncodeFrame(std::string_view payload);
    [[nodiscard]] static bool TryDecodeFrame(const std::vector<uint8_t>& buffer, size_t& consumed, std::string& payload);

    [[nodiscard]] static std::filesystem::path ApiRoot(const std::filesystem::path& projectOrEngineRoot);
    [[nodiscard]] static std::filesystem::path InboxDirectory(const std::filesystem::path& root);
    [[nodiscard]] static std::filesystem::path OutboxDirectory(const std::filesystem::path& root);
    [[nodiscard]] static std::filesystem::path EndpointFile(const std::filesystem::path& root);
    [[nodiscard]] static std::filesystem::path PidFile(const std::filesystem::path& root);
    [[nodiscard]] static std::string PipeName(const std::filesystem::path& root);
};

} // namespace we::runtime::core
