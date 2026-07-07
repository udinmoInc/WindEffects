#pragma once

#include "Services/ThumbnailRenderer.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <volk.h>

namespace we::editor::contentbrowser {

struct ThumbnailRequest {
    std::string id;
    std::string type;
    std::string path;
    bool isFolder = false;
    uint32_t folderVersion = 0;
    uint64_t sourceVersion = 0;
    int priority = 0;
};

struct ThumbnailResult {
    std::string id;
    BitmapRGBA bitmap;
    bool valid = false;
};

class ThumbnailManager {
public:
    ThumbnailManager();
    ~ThumbnailManager();

    void SetDiskCache(class DiskThumbnailCache* cache) { m_DiskCache = cache; }
    void SetFolderPreviewGenerator(class FolderPreviewGenerator* generator) { m_FolderPreview = generator; }

    void RequestThumbnail(const ThumbnailRequest& request);
    void CancelRequest(const std::string& id);
    void SetVisibleItems(const std::unordered_set<std::string>& visibleIds);
    void Invalidate(const std::string& id);
    void InvalidateAll();

    VkDescriptorSet GetCachedTexture(const std::string& id) const;
    bool HasCachedTexture(const std::string& id) const;

    void ProcessCompletedRequests(std::function<VkDescriptorSet(const BitmapRGBA&)> uploader,
                                  std::function<void(const std::string&, VkDescriptorSet)> onComplete);

private:
    void WorkerThread();
    std::string MakeCacheKey(const ThumbnailRequest& request) const;
    BitmapRGBA GenerateBitmap(const ThumbnailRequest& request);

    class DiskThumbnailCache* m_DiskCache = nullptr;
    class FolderPreviewGenerator* m_FolderPreview = nullptr;

    std::queue<ThumbnailRequest> m_RequestQueue;
    std::unordered_set<std::string> m_PendingIds;
    std::unordered_set<std::string> m_VisibleIds;
    std::unordered_set<std::string> m_CancelledIds;
    std::mutex m_RequestMutex;
    std::condition_variable m_RequestCv;

    std::queue<ThumbnailResult> m_ResultQueue;
    std::mutex m_ResultMutex;

    std::unordered_map<std::string, VkDescriptorSet> m_MemoryCache;
    mutable std::mutex m_CacheMutex;

    std::atomic<bool> m_Running{true};
    std::thread m_Worker;
};

} // namespace we::editor::contentbrowser
