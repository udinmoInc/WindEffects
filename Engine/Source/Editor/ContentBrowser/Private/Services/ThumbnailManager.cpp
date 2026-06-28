#include "ThumbnailManager.hpp"
#include <chrono>

namespace we::editor::contentbrowser {

ThumbnailManager::ThumbnailManager(std::unordered_map<std::string, VkDescriptorSet> defaultIcons)
    : m_DefaultIcons(std::move(defaultIcons)) 
{
    m_Worker = std::thread(&ThumbnailManager::WorkerThread, this);
}

ThumbnailManager::~ThumbnailManager() {
    m_Running = false;
    if (m_Worker.joinable()) {
        m_Worker.join();
    }
}

void ThumbnailManager::RequestThumbnail(const std::string& id, const std::string& type, const std::string& path) {
    std::lock_guard<std::mutex> lock(m_RequestMutex);
    m_RequestQueue.push({id, type, path});
}

void ThumbnailManager::ProcessCompletedRequests(std::function<void(const std::string&, VkDescriptorSet)> onComplete) {
    std::lock_guard<std::mutex> lock(m_ResultMutex);
    while (!m_ResultQueue.empty()) {
        auto result = m_ResultQueue.front();
        m_ResultQueue.pop();
        if (onComplete) {
            onComplete(result.id, result.textureId);
        }
    }
}

void ThumbnailManager::WorkerThread() {
    while (m_Running) {
        ThumbnailRequest request;
        bool hasRequest = false;
        
        {
            std::lock_guard<std::mutex> lock(m_RequestMutex);
            if (!m_RequestQueue.empty()) {
                request = m_RequestQueue.front();
                m_RequestQueue.pop();
                hasRequest = true;
            }
        }
        
        if (hasRequest) {
            // Simulate async generation time (100ms)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // For now, return the default icon for this type.
            // A full implementation would render the 3D model, load the image, etc.
            VkDescriptorSet texture = VK_NULL_HANDLE;
            std::string typeLower = request.type;
            for(auto& c : typeLower) c = ::tolower(c);
            
            if (m_DefaultIcons.find(typeLower) != m_DefaultIcons.end()) {
                texture = m_DefaultIcons[typeLower];
            } else {
                texture = m_DefaultIcons["generic"];
            }
            
            {
                std::lock_guard<std::mutex> lock(m_ResultMutex);
                m_ResultQueue.push({request.id, texture});
            }
        } else {
            // Sleep briefly to avoid burning CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

} // namespace we::editor::contentbrowser
