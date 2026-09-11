// ==============================================================================
// WindEffects — Editor — EditorRemoteControlSubsystem
// Named-pipe + inbox file-watcher bridge for AI/tools remote control.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

#include "Core/ApplicationFramework.h"
#include "Framework/IEditorLoopHost.h"

#include <atomic>
#include <filesystem>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace we::programs::editor {

class EditorRemoteControlSubsystem final : public we::runtime::core::ISubsystem {
public:
    explicit EditorRemoteControlSubsystem(IEditorLoopHost& host);
    ~EditorRemoteControlSubsystem() override;

    void Initialize() override;
    void Tick(float deltaTime) override;
    void Shutdown() override;

    const char* GetName() const override { return "EditorRemoteControlSubsystem"; }
    int GetPriority() const override { return 20; }

private:
    void RegisterBuiltinCommands();
    void EnsureDirectories();
    void WriteEndpointFiles();
    void DrainInbox();
    void DrainPendingQueue();
    void StartPipeServer();
    void StopPipeServer();
    void PipeServerLoop();

    [[nodiscard]] std::filesystem::path ResolveRoot() const;
    [[nodiscard]] std::string HandleRequestJson(const std::string& requestJson);

    IEditorLoopHost& m_Host;
    std::filesystem::path m_Root;
    std::atomic<bool> m_Running{false};
    std::thread m_PipeThread;

    std::mutex m_PendingMutex;
    std::queue<std::pair<std::string, std::string>> m_Pending; // requestJson, responsePath (optional)
};

} // namespace we::programs::editor
