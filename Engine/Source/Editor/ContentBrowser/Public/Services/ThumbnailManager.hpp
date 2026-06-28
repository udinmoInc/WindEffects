#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <volk.h>
#include <unordered_map>

namespace we::editor::contentbrowser {

struct ThumbnailRequest {
    std::string id;
    std::string type;
    std::string path;
};

struct ThumbnailResult {
    std::string id;
    VkDescriptorSet textureId = VK_NULL_HANDLE;
};

class ThumbnailManager {
public:
    ThumbnailManager(std::unordered_map<std::string, VkDescriptorSet> defaultIcons);
    ~ThumbnailManager();

    void RequestThumbnail(const std::string& id, const std::string& type, const std::string& path);
    
    // Call this every frame on the main thread
    void ProcessCompletedRequests(std::function<void(const std::string&, VkDescriptorSet)> onComplete);

private:
    void WorkerThread();

    std::unordered_map<std::string, VkDescriptorSet> m_DefaultIcons;

    std::queue<ThumbnailRequest> m_RequestQueue;
    std::mutex m_RequestMutex;
    
    std::queue<ThumbnailResult> m_ResultQueue;
    std::mutex m_ResultMutex;

    std::atomic<bool> m_Running{true};
    std::thread m_Worker;
};

} // namespace we::editor::contentbrowser
