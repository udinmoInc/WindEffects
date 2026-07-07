#include "Services/ThumbnailManager.h"
#include "Services/DiskThumbnailCache.h"
#include "Services/FolderPreviewGenerator.h"
#include "Services/ThumbnailRenderer.h"
#include "Registry/ContentAssetRegistry.h"

namespace we::editor::contentbrowser {

namespace {
constexpr uint64_t kThumbnailCacheSchema = 5; // folders use shared Lucide icon (no baked thumbnail)
}

ThumbnailManager::ThumbnailManager() {
    m_Worker = std::thread(&ThumbnailManager::WorkerThread, this);
}

ThumbnailManager::~ThumbnailManager() {
    m_Running = false;
    m_RequestCv.notify_all();
    if (m_Worker.joinable()) {
        m_Worker.join();
    }
}

std::string ThumbnailManager::MakeCacheKey(const ThumbnailRequest& request) const {
    std::hash<std::string> hasher;
    const auto hash = hasher(request.id + std::to_string(request.sourceVersion) + std::to_string(kThumbnailCacheSchema));
    return std::to_string(hash);
}

BitmapRGBA ThumbnailManager::GenerateBitmap(const ThumbnailRequest& request) {
    const auto cacheKey = MakeCacheKey(request);

    if (m_DiskCache) {
        if (auto cached = m_DiskCache->TryLoad(cacheKey, request.sourceVersion)) {
            return *cached;
        }
    }

    BitmapRGBA bitmap;
    if (request.isFolder) {
        return bitmap;
    }

    if (const auto* asset = ContentAssetRegistry::Get().FindById(request.id)) {
        bitmap = ThumbnailRenderer::Render(*asset);
    }

    if (!bitmap.pixels.empty() && m_DiskCache) {
        m_DiskCache->Save(cacheKey, request.sourceVersion, bitmap);
    }
    return bitmap;
}

void ThumbnailManager::RequestThumbnail(const ThumbnailRequest& request) {
    {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        if (m_MemoryCache.find(request.id) != m_MemoryCache.end()) return;
    }

    std::lock_guard<std::mutex> lock(m_RequestMutex);
    if (m_PendingIds.find(request.id) != m_PendingIds.end()) return;
    if (m_CancelledIds.erase(request.id) > 0) {
        // recently cancelled, allow re-request
    }

    ThumbnailRequest prioritized = request;
    prioritized.priority = m_VisibleIds.count(request.id) ? 1 : 0;
    m_RequestQueue.push(prioritized);
    m_PendingIds.insert(request.id);
    m_RequestCv.notify_one();
}

void ThumbnailManager::CancelRequest(const std::string& id) {
    std::lock_guard<std::mutex> lock(m_RequestMutex);
    m_CancelledIds.insert(id);
}

void ThumbnailManager::SetVisibleItems(const std::unordered_set<std::string>& visibleIds) {
    std::lock_guard<std::mutex> lock(m_RequestMutex);
    m_VisibleIds = visibleIds;
}

void ThumbnailManager::Invalidate(const std::string& id) {
    {
        std::lock_guard<std::mutex> lock(m_CacheMutex);
        m_MemoryCache.erase(id);
    }
    std::lock_guard<std::mutex> lock(m_RequestMutex);
    m_PendingIds.erase(id);
}

void ThumbnailManager::InvalidateAll() {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    m_MemoryCache.clear();
}

VkDescriptorSet ThumbnailManager::GetCachedTexture(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    auto it = m_MemoryCache.find(id);
    return it != m_MemoryCache.end() ? it->second : VK_NULL_HANDLE;
}

bool ThumbnailManager::HasCachedTexture(const std::string& id) const {
    std::lock_guard<std::mutex> lock(m_CacheMutex);
    return m_MemoryCache.find(id) != m_MemoryCache.end();
}

void ThumbnailManager::ProcessCompletedRequests(
    std::function<VkDescriptorSet(const BitmapRGBA&)> uploader,
    std::function<void(const std::string&, VkDescriptorSet)> onComplete)
{
    std::queue<ThumbnailResult> localResults;
    {
        std::lock_guard<std::mutex> lock(m_ResultMutex);
        std::swap(localResults, m_ResultQueue);
    }

    while (!localResults.empty()) {
        auto result = std::move(localResults.front());
        localResults.pop();
        if (!result.valid || !uploader) continue;

        VkDescriptorSet existing = GetCachedTexture(result.id);
        if (existing != VK_NULL_HANDLE) {
            if (onComplete) onComplete(result.id, existing);
            continue;
        }

        VkDescriptorSet texture = uploader(result.bitmap);
        if (texture == VK_NULL_HANDLE) continue;

        {
            std::lock_guard<std::mutex> lock(m_CacheMutex);
            m_MemoryCache[result.id] = texture;
        }
        if (onComplete) onComplete(result.id, texture);
    }
}

void ThumbnailManager::WorkerThread() {
    while (m_Running) {
        ThumbnailRequest request;
        {
            std::unique_lock<std::mutex> lock(m_RequestMutex);
            m_RequestCv.wait(lock, [&]() { return !m_Running || !m_RequestQueue.empty(); });
            if (!m_Running) break;
            if (m_RequestQueue.empty()) continue;

            request = m_RequestQueue.front();
            m_RequestQueue.pop();
            m_PendingIds.erase(request.id);
        }

        if (m_CancelledIds.count(request.id)) {
            m_CancelledIds.erase(request.id);
            continue;
        }

        ThumbnailResult result;
        result.id = request.id;
        result.bitmap = GenerateBitmap(request);
        result.valid = !result.bitmap.pixels.empty();

        {
            std::lock_guard<std::mutex> lock(m_ResultMutex);
            m_ResultQueue.push(std::move(result));
        }
    }
}

} // namespace we::editor::contentbrowser
