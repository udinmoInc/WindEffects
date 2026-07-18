#pragma once

#include "AssetPipeline/Export.h"
#include "Platform/IPlatform.h"
#include "Platform/Types.h"

#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace we::runtime::assetpipeline {

struct ASSETPIPELINE_API WatchedChange {
    std::string path;
    we::platform::DirectoryWatchAction action = we::platform::DirectoryWatchAction::Modified;
};

using SourceChangeCallback = std::function<void(const std::vector<WatchedChange>&)>;

/// Thin wrapper over Platform directory watching. Caller must PollEvents on the platform.
class ASSETPIPELINE_API SourceWatcher {
public:
    explicit SourceWatcher(we::platform::IPlatform* platform);
    ~SourceWatcher();

    SourceWatcher(const SourceWatcher&) = delete;
    SourceWatcher& operator=(const SourceWatcher&) = delete;

    [[nodiscard]] bool Start(const std::filesystem::path& directory, bool recursive = true);
    void Stop();
    [[nodiscard]] bool IsWatching() const noexcept;

    void SetCallback(SourceChangeCallback callback);

    /// Drain pending Platform DirectoryChangeEvents for our watcher id.
    void Poll(we::platform::IPlatform& platform);

private:
    we::platform::IPlatform* m_Platform = nullptr;
    we::platform::DirectoryWatcherId m_WatcherId = we::platform::DirectoryWatcherId::Invalid;
    SourceChangeCallback m_Callback;
    mutable std::mutex m_Mutex;
    std::vector<WatchedChange> m_Pending;
};

} // namespace we::runtime::assetpipeline
