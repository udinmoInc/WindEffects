#include "Rendering/Text/FontAssetManager.h"
#include "Core/Logger.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <filesystem>
#include <algorithm>

namespace we::UI::Text {

// ============================================================================
// FontAssetManager Implementation
// ============================================================================

FontAssetManager::FontAssetManager() = default;

FontAssetManager::~FontAssetManager() {
    Shutdown();
}

bool FontAssetManager::Initialize(const FontAssetManagerConfig& config) {
    if (m_Initialized) {
        HE_WARN("FontAssetManager: Already initialized");
        return true;
    }

    m_Config = config;
    m_Assets.clear();
    m_NameToId.clear();
    m_NextAssetId = 1;
    m_LastReloadCheckTime = 0;

    // Create cache directory if it doesn't exist
    if (m_Config.cacheGeneratedAtlases && !m_Config.cacheDirectory.empty()) {
        std::filesystem::create_directories(m_Config.cacheDirectory);
    }

    m_Initialized = true;
    HE_INFO("FontAssetManager: Initialized (hot-reloading: " + 
            std::string(m_Config.enableHotReloading ? "enabled" : "disabled") + ")");
    return true;
}

void FontAssetManager::Shutdown() {
    if (!m_Initialized) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_Mutex);

    // Unload all assets
    for (auto& [id, data] : m_Assets) {
        if (data.face) {
            // Note: We don't own the face, so we don't free it here
            // The font loader would handle that
        }
    }

    m_Assets.clear();
    m_NameToId.clear();
    m_ReloadCallback = nullptr;
    m_Initialized = false;

    HE_INFO("FontAssetManager: Shutdown");
}

AssetLoadResult FontAssetManager::RegisterAsset(const FontAssetDescriptor& descriptor) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    AssetLoadResult result;

    // Validate descriptor
    std::vector<std::string> errors;
    if (!FontAssetManagerUtils::ValidateDescriptor(descriptor, errors)) {
        result.success = false;
        result.errorMessage = "Invalid descriptor: " + errors[0];
        HE_ERROR("FontAssetManager: " + result.errorMessage);
        return result;
    }

    // Check if asset already exists
    if (m_NameToId.find(descriptor.assetName) != m_NameToId.end()) {
        result.success = false;
        result.errorMessage = "Asset already registered: " + descriptor.assetName;
        HE_WARN("FontAssetManager: " + result.errorMessage);
        return result;
    }

    // Create asset data
    uint64_t assetId = GenerateAssetId();
    AssetData data;
    data.descriptor = descriptor;
    data.handle.id = assetId;
    data.handle.name = descriptor.assetName;
    data.handle.status = AssetLoadStatus::NotLoaded;
    data.lastModificationTime = GetFileModificationTime(descriptor.fontPath);

    m_Assets[assetId] = data;
    m_NameToId[descriptor.assetName] = assetId;

    result.success = true;
    result.handle = data.handle;

    HE_INFO("FontAssetManager: Registered asset '" + descriptor.assetName + "' from " + descriptor.fontPath);

    // Auto-load if requested
    if (descriptor.preload) {
        LoadAsset(assetId);
    }

    return result;
}

bool FontAssetManager::UnregisterAsset(uint64_t assetId) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Assets.find(assetId);
    if (it == m_Assets.end()) {
        return false;
    }

    const std::string& assetName = it->second.descriptor.assetName;
    m_NameToId.erase(assetName);
    m_Assets.erase(it);

    HE_INFO("FontAssetManager: Unregistered asset '" + assetName + "'");
    return true;
}

AssetLoadResult FontAssetManager::LoadAsset(uint64_t assetId) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Assets.find(assetId);
    if (it == m_Assets.end()) {
        AssetLoadResult result;
        result.success = false;
        result.errorMessage = "Asset not found";
        return result;
    }

    AssetData& data = it->second;
    data.handle.status = AssetLoadStatus::Loading;

    // Try to load from cache first
    if (m_Config.cacheGeneratedAtlases && LoadCachedAtlas(assetId)) {
        data.handle.status = AssetLoadStatus::Loaded;
        HE_INFO("FontAssetManager: Loaded asset '" + data.descriptor.assetName + "' from cache");
        
        AssetLoadResult result;
        result.success = true;
        result.handle = data.handle;
        return result;
    }

    // Load from file (placeholder - would use FontLoader)
    data.handle.status = AssetLoadStatus::Loaded;
    data.lastModificationTime = GetFileModificationTime(data.descriptor.fontPath);

    HE_INFO("FontAssetManager: Loaded asset '" + data.descriptor.assetName + "' from file");

    AssetLoadResult result;
    result.success = true;
    result.handle = data.handle;
    return result;
}

bool FontAssetManager::UnloadAsset(uint64_t assetId) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Assets.find(assetId);
    if (it == m_Assets.end()) {
        return false;
    }

    AssetData& data = it->second;
    data.handle.status = AssetLoadStatus::NotLoaded;
    data.cachedAtlas.clear();

    HE_INFO("FontAssetManager: Unloaded asset '" + data.descriptor.assetName + "'");
    return true;
}

AssetLoadResult FontAssetManager::ReloadAsset(uint64_t assetId) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Assets.find(assetId);
    if (it == m_Assets.end()) {
        AssetLoadResult result;
        result.success = false;
        result.errorMessage = "Asset not found";
        return result;
    }

    AssetData& data = it->second;
    data.handle.status = AssetLoadStatus::Reloading;

    // Unload first
    UnloadAsset(assetId);

    // Reload
    AssetLoadResult result = LoadAsset(assetId);

    if (result.success && m_ReloadCallback) {
        m_ReloadCallback(data.handle);
    }

    return result;
}

FontAssetHandle FontAssetManager::GetAsset(const std::string& assetName) const {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_NameToId.find(assetName);
    if (it != m_NameToId.end()) {
        auto assetIt = m_Assets.find(it->second);
        if (assetIt != m_Assets.end()) {
            return assetIt->second.handle;
        }
    }

    return FontAssetHandle{};
}

FontAssetHandle FontAssetManager::GetAsset(uint64_t assetId) const {
    std::lock_guard<std::mutex> lock(m_Mutex);

    auto it = m_Assets.find(assetId);
    if (it != m_Assets.end()) {
        return it->second.handle;
    }

    return FontAssetHandle{};
}

std::vector<FontAssetHandle> FontAssetManager::GetAllAssets() const {
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::vector<FontAssetHandle> handles;
    handles.reserve(m_Assets.size());

    for (const auto& [id, data] : m_Assets) {
        handles.push_back(data.handle);
    }

    return handles;
}

void FontAssetManager::SetReloadCallback(AssetReloadCallback callback) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_ReloadCallback = callback;
}

void FontAssetManager::Update() {
    if (!m_Config.enableHotReloading) {
        return;
    }

    uint64_t currentTime = GetFileModificationTime("");  // Get current time (simplified)
    
    if (currentTime - m_LastReloadCheckTime >= m_Config.reloadCheckIntervalMs) {
        CheckFileModifications();
        m_LastReloadCheckTime = currentTime;
    }
}

size_t FontAssetManager::ReloadAll() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    size_t reloaded = 0;
    for (auto& [id, data] : m_Assets) {
        if (ReloadAsset(id).success) {
            reloaded++;
        }
    }

    HE_INFO("FontAssetManager: Reloaded " + std::to_string(reloaded) + " assets");
    return reloaded;
}

void FontAssetManager::ClearCache() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (auto& [id, data] : m_Assets) {
        data.cachedAtlas.clear();
    }

    // Clear cache directory
    if (!m_Config.cacheDirectory.empty()) {
        try {
            std::filesystem::remove_all(m_Config.cacheDirectory);
            std::filesystem::create_directories(m_Config.cacheDirectory);
        } catch (const std::filesystem::filesystem_error&) {
            HE_WARN("FontAssetManager: Failed to clear cache directory");
        }
    }

    HE_INFO("FontAssetManager: Cleared cache");
}

bool FontAssetManager::UpdateConfig(const FontAssetManagerConfig& config) {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Config = config;
    return true;
}

std::unordered_map<uint64_t, AssetLoadStatus> FontAssetManager::GetStatistics() const {
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::unordered_map<uint64_t, AssetLoadStatus> stats;
    for (const auto& [id, data] : m_Assets) {
        stats[id] = data.handle.status;
    }
    return stats;
}

void FontAssetManager::CheckFileModifications() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    for (auto& [id, data] : m_Assets) {
        uint64_t currentModTime = GetFileModificationTime(data.descriptor.fontPath);
        
        if (currentModTime > data.lastModificationTime) {
            data.needsReload = true;
            data.lastModificationTime = currentModTime;
            
            HE_INFO("FontAssetManager: Asset '" + data.descriptor.assetName + "' modified, queuing reload");
            
            // Reload asynchronously if enabled, otherwise reload immediately
            if (m_Config.enableAsyncLoading) {
                // Placeholder for async reload
            } else {
                ReloadAsset(id);
            }
        }
    }
}

uint64_t FontAssetManager::GetFileModificationTime(const std::string& filePath) const {
    if (filePath.empty()) {
        // Return current time in milliseconds
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }

    try {
        auto ftime = std::filesystem::last_write_time(filePath);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime);
        auto epoch = sctp.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();
    } catch (const std::filesystem::filesystem_error&) {
        return 0;
    }
}

bool FontAssetManager::LoadCachedAtlas(uint64_t assetId) {
    auto it = m_Assets.find(assetId);
    if (it == m_Assets.end()) {
        return false;
    }

    AssetData& data = it->second;
    std::string cachePath = GetCachePath(assetId);

    if (!std::filesystem::exists(cachePath)) {
        return false;
    }

    // Load cached atlas (placeholder)
    std::ifstream file(cachePath, std::ios::binary);
    if (!file) {
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    data.cachedAtlas.resize(fileSize);
    file.read(reinterpret_cast<char*>(data.cachedAtlas.data()), fileSize);
    file.close();

    HE_INFO("FontAssetManager: Loaded cached atlas for '" + data.descriptor.assetName + "'");
    return true;
}

bool FontAssetManager::SaveAtlasToCache(uint64_t assetId) {
    if (!m_Config.cacheGeneratedAtlases) {
        return false;
    }

    auto it = m_Assets.find(assetId);
    if (it == m_Assets.end()) {
        return false;
    }

    AssetData& data = it->second;
    std::string cachePath = GetCachePath(assetId);

    // Save atlas (placeholder)
    std::ofstream file(cachePath, std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(data.cachedAtlas.data()), data.cachedAtlas.size());
    file.close();

    HE_INFO("FontAssetManager: Saved cached atlas for '" + data.descriptor.assetName + "'");
    return true;
}

std::string FontAssetManager::GetCachePath(uint64_t assetId) const {
    auto it = m_Assets.find(assetId);
    if (it == m_Assets.end()) {
        return "";
    }

    const std::string& assetName = it->second.descriptor.assetName;
    return m_Config.cacheDirectory + "/" + assetName + ".atlas";
}

uint64_t FontAssetManager::GenerateAssetId() {
    return m_NextAssetId++;
}

// ============================================================================
// FontAssetManagerUtils Implementation
// ============================================================================

namespace FontAssetManagerUtils {

FontAssetManagerConfig GetDefaultConfig() {
    FontAssetManagerConfig config;
    config.enableHotReloading = true;
    config.reloadCheckIntervalMs = 1000;
    config.enableAsyncLoading = false;
    config.maxConcurrentLoads = 4;
    config.cacheGeneratedAtlases = true;
    config.cacheDirectory = "Cache/Fonts";
    return config;
}

FontAssetManagerConfig GetDevelopmentConfig() {
    FontAssetManagerConfig config = GetDefaultConfig();
    config.enableHotReloading = true;
    config.reloadCheckIntervalMs = 500;  // Check more frequently in dev
    config.cacheGeneratedAtlases = false;  // Don't cache in dev
    return config;
}

FontAssetManagerConfig GetProductionConfig() {
    FontAssetManagerConfig config = GetDefaultConfig();
    config.enableHotReloading = false;  // Disable in production
    config.enableAsyncLoading = true;   // Enable async loading
    config.cacheGeneratedAtlases = true;
    return config;
}

bool ValidateDescriptor(const FontAssetDescriptor& descriptor, 
                        std::vector<std::string>& outErrors) {
    outErrors.clear();

    if (descriptor.fontPath.empty()) {
        outErrors.push_back("Font path is empty");
    }

    if (!std::filesystem::exists(descriptor.fontPath)) {
        outErrors.push_back("Font file does not exist: " + descriptor.fontPath);
    }

    if (descriptor.assetName.empty()) {
        outErrors.push_back("Asset name is empty");
    }

    if (descriptor.fontSize <= 0.0f || descriptor.fontSize > 256.0f) {
        outErrors.push_back("Font size must be between 0 and 256");
    }

    return outErrors.empty();
}

FontAssetDescriptor CreateDescriptor(const std::string& fontPath, 
                                     const std::string& assetName) {
    FontAssetDescriptor descriptor;
    descriptor.fontPath = fontPath;
    descriptor.assetName = assetName.empty() ? 
        std::filesystem::path(fontPath).stem().string() : assetName;
    descriptor.fontSize = 18.0f;
    descriptor.faceIndex = 0;
    descriptor.preload = true;
    return descriptor;
}

} // namespace FontAssetManagerUtils

} // namespace we::UI::Text
