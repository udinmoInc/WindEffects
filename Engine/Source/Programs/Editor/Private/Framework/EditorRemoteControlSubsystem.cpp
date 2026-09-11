// ==============================================================================
// WindEffects — Editor — EditorRemoteControlSubsystem
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Framework/EditorRemoteControlSubsystem.h"

#include "Core/AssetCatalog.h"
#include "Core/AssetRegistry.h"
#include "Core/DiagnosticMacros.h"
#include "Core/EngineApiRegistry.h"
#include "Core/EngineRemoteProtocol.h"
#include "Core/FrameCounter.h"
#include "Core/Paths.h"
#include "KindUI/Core/UIRepaintGate.h"
#include "KindUI/Theming/PaletteRuntime.h"
#include "WindEffects/Editor/UI/Core/EditorPerfStats.h"

#include <chrono>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

#include "Platform/UndefWin32Macros.h"

namespace we::programs::editor {
namespace {

std::string ReadFileText(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void WriteFileText(const std::filesystem::path& path, const std::string& text) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

} // namespace

EditorRemoteControlSubsystem::EditorRemoteControlSubsystem(IEditorLoopHost& host)
    : m_Host(host) {}

EditorRemoteControlSubsystem::~EditorRemoteControlSubsystem() {
    StopPipeServer();
}

void EditorRemoteControlSubsystem::Initialize() {
    if (!m_Host.HostIsRemoteApiEnabled()) {
        HE_INFO("[EditorRemote] Disabled (--no-remote)");
        return;
    }
    m_Root = ResolveRoot();
    EnsureDirectories();
    RegisterBuiltinCommands();
    WriteEndpointFiles();
    m_Running.store(true);
    StartPipeServer();
    HE_INFO("[EditorRemote] Ready — inbox=" + we::core::PathService::ToUtf8(
        we::runtime::core::EngineRemoteProtocol::InboxDirectory(m_Root)));
}

void EditorRemoteControlSubsystem::Tick(float /*deltaTime*/) {
    if (!m_Running.load()) {
        return;
    }
    DrainInbox();
    DrainPendingQueue();
}

void EditorRemoteControlSubsystem::Shutdown() {
    StopPipeServer();
    auto& api = we::runtime::core::EngineApiRegistry::Get();
    for (const auto& name : {
             "api.list", "status", "perf.dump", "theme.reload", "assets.reload",
             "layout.reload", "config.set", "config.get", "shader.request"}) {
        api.Unregister(name);
    }
    HE_INFO("[EditorRemote] Shutdown");
}

std::filesystem::path EditorRemoteControlSubsystem::ResolveRoot() const {
    auto& paths = we::core::PathService::Get();
    if (!paths.ProjectRoot().empty()) {
        return paths.ProjectRoot();
    }
    if (!paths.EngineRoot().empty()) {
        return paths.EngineRoot();
    }
    return paths.ExecutableDirectory();
}

void EditorRemoteControlSubsystem::EnsureDirectories() {
    std::error_code ec;
    std::filesystem::create_directories(
        we::runtime::core::EngineRemoteProtocol::InboxDirectory(m_Root), ec);
    std::filesystem::create_directories(
        we::runtime::core::EngineRemoteProtocol::OutboxDirectory(m_Root), ec);
}

void EditorRemoteControlSubsystem::WriteEndpointFiles() {
    const auto pipe = we::runtime::core::EngineRemoteProtocol::PipeName(m_Root);
    const auto endpoint = we::runtime::core::EngineRemoteProtocol::EndpointFile(m_Root);
    const auto pidPath = we::runtime::core::EngineRemoteProtocol::PidFile(m_Root);
#if defined(_WIN32)
    WriteFileText(endpoint, "pipe:\\\\.\\pipe\\" + pipe + "\n");
#else
    WriteFileText(endpoint, "inbox:" + we::core::PathService::ToUtf8(
        we::runtime::core::EngineRemoteProtocol::InboxDirectory(m_Root)) + "\n");
#endif
    WriteFileText(pidPath, std::to_string(
#if defined(_WIN32)
        static_cast<unsigned long>(GetCurrentProcessId())
#else
        static_cast<unsigned long>(::getpid())
#endif
        ) + "\n");
}

void EditorRemoteControlSubsystem::RegisterBuiltinCommands() {
    auto& api = we::runtime::core::EngineApiRegistry::Get();

    api.Register("api.list", [](const std::string&) {
#if WE_HAS_NLOHMANN_JSON
        nlohmann::json out = nlohmann::json::array();
        for (const auto& name : we::runtime::core::EngineApiRegistry::Get().ListCommands()) {
            out.push_back(name);
        }
        return nlohmann::json{{"commands", out}}.dump();
#else
        return std::string("{\"commands\":[]}");
#endif
    });

    api.Register("status", [this](const std::string&) {
#if WE_HAS_NLOHMANN_JSON
        auto& paths = we::core::PathService::Get();
        const auto catalog = we::runtime::core::AssetCatalogService::Load();
        nlohmann::json out = {
            {"running", m_Host.IsHostRunning()},
            {"frame", we::runtime::core::FrameCounter::GetFrameNumber()},
            {"engineRoot", we::core::PathService::ToUtf8(paths.EngineRoot())},
            {"projectRoot", we::core::PathService::ToUtf8(paths.ProjectRoot())},
            {"activeTheme", we::runtime::core::AssetCatalogService::GetActiveThemeName(catalog)},
            {"themePath", we::core::PathService::ToUtf8(
                we::runtime::core::AssetCatalogService::ResolveThemeConfigPath(catalog))},
            {"remoteInbox", we::core::PathService::ToUtf8(
                we::runtime::core::EngineRemoteProtocol::InboxDirectory(m_Root))},
        };
        return out.dump();
#else
        (void)this;
        return std::string("{\"running\":true}");
#endif
    });

    api.Register("perf.dump", [](const std::string&) {
#if WE_HAS_NLOHMANN_JSON
        const auto& sample = ::we::editor::services::EditorPerfStats::Get().Last();
        nlohmann::json out = {
            {"fps", ::we::editor::services::EditorPerfStats::Get().AverageFps()},
            {"frameMs", sample.frameMs},
            {"tickMs", sample.tickMs},
            {"layoutMs", sample.layoutMs},
            {"uiBuildMs", sample.uiBuildMs},
            {"sceneMs", sample.sceneMs},
            {"presentMs", sample.presentMs},
            {"uiVertices", sample.uiVertices},
            {"uiBatches", sample.uiBatches},
        };
        return out.dump();
#else
        return std::string("{}");
#endif
    });

    api.Register("theme.reload", [](const std::string& argsJson) {
#if WE_HAS_NLOHMANN_JSON
        try {
            const auto args = nlohmann::json::parse(argsJson.empty() ? "{}" : argsJson);
            if (args.contains("theme") && args["theme"].is_string()) {
                we::runtime::core::AssetCatalogService::SetActiveThemeOverride(args["theme"].get<std::string>());
            }
        } catch (...) {
        }
#endif
        const bool ok = we::runtime::kindui::palette::ForceReloadActiveThemePalette();
        we::runtime::kindui::UIRepaintGate::Request();
#if WE_HAS_NLOHMANN_JSON
        return nlohmann::json{{"reloaded", ok}}.dump();
#else
        return std::string("{\"reloaded\":") + (ok ? "true" : "false") + "}";
#endif
    });

    api.Register("assets.reload", [](const std::string&) {
        const bool ok = we::core::AssetRegistry::Get().LoadDefaultEditorAssets();
#if WE_HAS_NLOHMANN_JSON
        return nlohmann::json{{"ok", ok}}.dump();
#else
        return std::string("{\"ok\":") + (ok ? "true" : "false") + "}";
#endif
    });

    api.Register("layout.reload", [this](const std::string&) {
        m_Host.HostReloadLayout();
        we::runtime::kindui::UIRepaintGate::Request();
        return std::string("{\"ok\":true}");
    });

    api.Register("config.set", [](const std::string& argsJson) {
#if WE_HAS_NLOHMANN_JSON
        const auto args = nlohmann::json::parse(argsJson.empty() ? "{}" : argsJson);
        const std::string key = args.value("key", "");
        const std::string value = args.contains("value")
            ? (args["value"].is_string() ? args["value"].get<std::string>() : args["value"].dump())
            : "";
        if (key.empty()) {
            throw std::runtime_error("config.set requires args.key");
        }
        we::runtime::core::EngineApiRegistry::Get().SetConfigValue(key, value);
        if (key == "theme") {
            we::runtime::core::AssetCatalogService::SetActiveThemeOverride(value);
        }
        return nlohmann::json{{"key", key}, {"value", value}}.dump();
#else
        (void)argsJson;
        return std::string("{}");
#endif
    });

    api.Register("config.get", [](const std::string& argsJson) {
#if WE_HAS_NLOHMANN_JSON
        const auto args = nlohmann::json::parse(argsJson.empty() ? "{}" : argsJson);
        if (args.contains("key") && args["key"].is_string()) {
            const auto key = args["key"].get<std::string>();
            return nlohmann::json{
                {"key", key},
                {"value", we::runtime::core::EngineApiRegistry::Get().GetConfigValue(key)}
            }.dump();
        }
        nlohmann::json all = nlohmann::json::object();
        for (const auto& [k, v] : we::runtime::core::EngineApiRegistry::Get().GetAllConfigValues()) {
            all[k] = v;
        }
        return nlohmann::json{{"values", all}}.dump();
#else
        (void)argsJson;
        return std::string("{}");
#endif
    });

    api.Register("shader.request", [this](const std::string& argsJson) {
        const auto stampDir = we::runtime::core::EngineRemoteProtocol::ApiRoot(m_Root) / "requests";
        std::error_code ec;
        std::filesystem::create_directories(stampDir, ec);
        const auto stamp = stampDir / "shader-compile.stamp";
        WriteFileText(stamp, argsJson.empty() ? "{}" : argsJson);
#if WE_HAS_NLOHMANN_JSON
        return nlohmann::json{
            {"queued", true},
            {"stamp", we::core::PathService::ToUtf8(stamp)},
            {"note", "Write stamp for IgniteBT/tools; runtime loads prebuilt bytecode."}
        }.dump();
#else
        return std::string("{\"queued\":true}");
#endif
    });
}

std::string EditorRemoteControlSubsystem::HandleRequestJson(const std::string& requestJson) {
    we::runtime::core::EngineRemoteRequest request;
    if (!we::runtime::core::EngineRemoteProtocol::TryParseRequest(requestJson, request)) {
        we::runtime::core::EngineRemoteResponse bad;
        bad.ok = false;
        bad.error = "Invalid request JSON";
        return we::runtime::core::EngineRemoteProtocol::SerializeResponse(bad);
    }

    const std::string invoked =
        we::runtime::core::EngineApiRegistry::Get().Invoke(request.command, request.argsJson);

    we::runtime::core::EngineRemoteResponse response;
    response.id = request.id;
#if WE_HAS_NLOHMANN_JSON
    try {
        const auto wrapped = nlohmann::json::parse(invoked);
        response.ok = wrapped.value("ok", false);
        response.error = wrapped.contains("error") && !wrapped["error"].is_null()
            ? wrapped["error"].get<std::string>()
            : "";
        response.resultJson = wrapped.contains("result") ? wrapped["result"].dump() : "{}";
    } catch (...) {
        response.ok = false;
        response.error = "Malformed handler response";
        response.resultJson = "{}";
    }
#else
    response.ok = true;
    response.resultJson = invoked;
#endif
    return we::runtime::core::EngineRemoteProtocol::SerializeResponse(response);
}

void EditorRemoteControlSubsystem::DrainInbox() {
    const auto inbox = we::runtime::core::EngineRemoteProtocol::InboxDirectory(m_Root);
    const auto outbox = we::runtime::core::EngineRemoteProtocol::OutboxDirectory(m_Root);
    std::error_code ec;
    if (!std::filesystem::exists(inbox, ec)) {
        return;
    }
    for (const auto& entry : std::filesystem::directory_iterator(inbox, ec)) {
        if (ec || !entry.is_regular_file(ec)) {
            continue;
        }
        const auto path = entry.path();
        if (path.extension() != ".json") {
            continue;
        }
        const std::string requestJson = ReadFileText(path);
        const std::string responseJson = HandleRequestJson(requestJson);
        auto outPath = outbox / (path.stem().string() + ".rsp.json");
        WriteFileText(outPath, responseJson);
        std::filesystem::remove(path, ec);
    }
}

void EditorRemoteControlSubsystem::DrainPendingQueue() {
    std::queue<std::pair<std::string, std::string>> local;
    {
        std::lock_guard lock(m_PendingMutex);
        std::swap(local, m_Pending);
    }
    while (!local.empty()) {
        auto [requestJson, responsePath] = std::move(local.front());
        local.pop();
        const std::string responseJson = HandleRequestJson(requestJson);
        if (!responsePath.empty()) {
            WriteFileText(std::filesystem::path(responsePath), responseJson);
        }
    }
}

void EditorRemoteControlSubsystem::StartPipeServer() {
#if defined(_WIN32)
    m_PipeThread = std::thread([this]() { PipeServerLoop(); });
#endif
}

void EditorRemoteControlSubsystem::StopPipeServer() {
    m_Running.store(false);
#if defined(_WIN32)
    // Connect briefly to unblock ConnectNamedPipe.
    const auto pipeName = "\\\\.\\pipe\\" + we::runtime::core::EngineRemoteProtocol::PipeName(m_Root);
    HANDLE client = CreateFileA(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
        OPEN_EXISTING, 0, nullptr);
    if (client != INVALID_HANDLE_VALUE) {
        CloseHandle(client);
    }
#endif
    if (m_PipeThread.joinable()) {
        m_PipeThread.join();
    }
}

void EditorRemoteControlSubsystem::PipeServerLoop() {
#if defined(_WIN32)
    const std::string pipeName = "\\\\.\\pipe\\" + we::runtime::core::EngineRemoteProtocol::PipeName(m_Root);
    while (m_Running.load()) {
        HANDLE pipe = CreateNamedPipeA(
            pipeName.c_str(),
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
            1,
            64 * 1024,
            64 * 1024,
            0,
            nullptr);
        if (pipe == INVALID_HANDLE_VALUE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            continue;
        }
        const BOOL connected = ConnectNamedPipe(pipe, nullptr)
            ? TRUE
            : (GetLastError() == ERROR_PIPE_CONNECTED);
        if (!connected || !m_Running.load()) {
            CloseHandle(pipe);
            continue;
        }

        std::vector<uint8_t> buffer;
        buffer.reserve(4096);
        uint8_t chunk[1024];
        DWORD read = 0;
        while (ReadFile(pipe, chunk, sizeof(chunk), &read, nullptr) && read > 0) {
            buffer.insert(buffer.end(), chunk, chunk + read);
            size_t consumed = 0;
            std::string payload;
            if (we::runtime::core::EngineRemoteProtocol::TryDecodeFrame(buffer, consumed, payload)) {
                const std::string responseJson = HandleRequestJson(payload);
                const auto frame = we::runtime::core::EngineRemoteProtocol::EncodeFrame(responseJson);
                DWORD written = 0;
                WriteFile(pipe, frame.data(), static_cast<DWORD>(frame.size()), &written, nullptr);
                break;
            }
            if (buffer.size() > 4 * 1024 * 1024) {
                break;
            }
        }
        FlushFileBuffers(pipe);
        DisconnectNamedPipe(pipe);
        CloseHandle(pipe);
    }
#else
    while (m_Running.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
#endif
}

} // namespace we::programs::editor
