#include "AssetPipeline/SourceWatcher.h"

#include "Platform/Events.h"

namespace we::runtime::assetpipeline {

SourceWatcher::SourceWatcher(we::platform::IPlatform* platform)
    : m_Platform(platform) {}

SourceWatcher::~SourceWatcher() {
    Stop();
}

bool SourceWatcher::Start(const std::filesystem::path& directory, bool recursive) {
    Stop();
    if (!m_Platform) {
        return false;
    }
    auto result = m_Platform->WatchDirectory(directory.string(), recursive);
    if (!result) {
        return false;
    }
    m_WatcherId = result.value;

    m_Platform->AddEventHandler([this](const we::platform::PlatformEvent& event) {
        if (!std::holds_alternative<we::platform::DirectoryChangeEvent>(event)) {
            return;
        }
        const auto& change = std::get<we::platform::DirectoryChangeEvent>(event);
        if (change.watcher != m_WatcherId) {
            return;
        }
        WatchedChange w{};
        w.path = change.path;
        w.action = change.action;
        std::lock_guard lock(m_Mutex);
        m_Pending.push_back(std::move(w));
    });
    return true;
}

void SourceWatcher::Stop() {
    if (m_Platform && m_WatcherId != we::platform::DirectoryWatcherId::Invalid) {
        (void)m_Platform->UnwatchDirectory(m_WatcherId);
        m_WatcherId = we::platform::DirectoryWatcherId::Invalid;
    }
    std::lock_guard lock(m_Mutex);
    m_Pending.clear();
}

bool SourceWatcher::IsWatching() const noexcept {
    return m_WatcherId != we::platform::DirectoryWatcherId::Invalid;
}

void SourceWatcher::SetCallback(SourceChangeCallback callback) {
    std::lock_guard lock(m_Mutex);
    m_Callback = std::move(callback);
}

void SourceWatcher::Poll(we::platform::IPlatform& platform) {
    (void)platform;
    std::vector<WatchedChange> batch;
    SourceChangeCallback cb;
    {
        std::lock_guard lock(m_Mutex);
        batch.swap(m_Pending);
        cb = m_Callback;
    }
    if (!batch.empty() && cb) {
        cb(batch);
    }
}

} // namespace we::runtime::assetpipeline
