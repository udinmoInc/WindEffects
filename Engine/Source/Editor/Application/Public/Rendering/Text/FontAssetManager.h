#pragma once

#include "Application/Export.h"

#include "Rendering/Text/FontLoader.h"
#include "Rendering/Text/GlyphManager.h"
#include "Rendering/Text/MsdfAtlasGenerator.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <mutex>

namespace we::UI::Text {

/**
 * @brief Asset loading status
 */
enum class AssetLoadStatus : uint8_t {
    NotLoaded,
    Loading,
    Loaded,
    Failed,
    Reloading
};

/**
 * @brief Font asset descriptor
 */
struct FontAssetDescriptor {
    std::string fontPath;
    std::string assetName;
    float fontSize = 18.0f;
    uint32_t faceIndex = 0;
    bool preload = true;
};

/**
 * @brief Font asset handle
 */
struct FontAssetHandle {
    uint64_t id = 0;
    std::string name;
    AssetLoadStatus status = AssetLoadStatus::NotLoaded;
};

/**
 * @brief Asset load result
 */
struct AssetLoadResult {
    bool success = false;
    std::string errorMessage;
    FontAssetHandle handle;
};

/**
 * @brief Asset reload callback
 */
using AssetReloadCallback = std::function<void(const FontAssetHandle&)>;

/**
 * @brief Font asset manager configuration
 */
struct FontAssetManagerConfig {
    bool enableHotReloading = true;
    uint32_t reloadCheckIntervalMs = 1000;  // Check for file changes every second
    bool enableAsyncLoading = false;
    size_t maxConcurrentLoads = 4;
    bool cacheGeneratedAtlases = true;
    std::string cacheDirectory = "Cache/Fonts";
};

/**
 * @brief Font asset manager interface
 * 
 * Manages font assets with hot-reloading, caching, and async loading support.
 * Tracks font files and automatically reloads when changed.
 */
class APPLICATION_API IFontAssetManager {
public:
    virtual ~IFontAssetManager() = default;

    /**
     * @brief Initialize the asset manager
     * @param config Asset manager configuration
     * @return true if successful, false otherwise
     */
    virtual bool Initialize(const FontAssetManagerConfig& config) = 0;

    /**
     * @brief Shutdown the asset manager
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Register a font asset
     * @param descriptor Font asset descriptor
     * @return AssetLoadResult
     */
    virtual AssetLoadResult RegisterAsset(const FontAssetDescriptor& descriptor) = 0;

    /**
     * @brief Unregister a font asset
     * @param assetId Asset ID
     * @return true if successful, false otherwise
     */
    virtual bool UnregisterAsset(uint64_t assetId) = 0;

    /**
     * @brief Load a font asset
     * @param assetId Asset ID
     * @return AssetLoadResult
     */
    virtual AssetLoadResult LoadAsset(uint64_t assetId) = 0;

    /**
     * @brief Unload a font asset
     * @param assetId Asset ID
     * @return true if successful, false otherwise
     */
    virtual bool UnloadAsset(uint64_t assetId) = 0;

    /**
     * @brief Reload a font asset
     * @param assetId Asset ID
     * @return AssetLoadResult
     */
    virtual AssetLoadResult ReloadAsset(uint64_t assetId) = 0;

    /**
     * @brief Get asset handle by name
     * @param assetName Asset name
     * @return FontAssetHandle (invalid if not found)
     */
    virtual FontAssetHandle GetAsset(const std::string& assetName) const = 0;

    /**
     * @brief Get asset handle by ID
     * @param assetId Asset ID
     * @return FontAssetHandle (invalid if not found)
     */
    virtual FontAssetHandle GetAsset(uint64_t assetId) const = 0;

    /**
     * @brief Get all registered assets
     * @return Vector of asset handles
     */
    virtual std::vector<FontAssetHandle> GetAllAssets() const = 0;

    /**
     * @brief Set reload callback
     * @param callback Callback function
     */
    virtual void SetReloadCallback(AssetReloadCallback callback) = 0;

    /**
     * @brief Update the asset manager (call each frame)
     */
    virtual void Update() = 0;

    /**
     * @brief Force reload all assets
     * @return Number of assets reloaded
     */
    virtual size_t ReloadAll() = 0;

    /**
     * @brief Clear asset cache
     */
    virtual void ClearCache() = 0;

    /**
     * @brief Get current configuration
     * @return FontAssetManagerConfig structure
     */
    virtual FontAssetManagerConfig GetConfig() const = 0;

    /**
     * @brief Update configuration
     * @param config New configuration
     * @return true if successful, false otherwise
     */
    virtual bool UpdateConfig(const FontAssetManagerConfig& config) = 0;

    /**
     * @brief Get asset statistics
     * @return Map of asset ID to load status
     */
    virtual std::unordered_map<uint64_t, AssetLoadStatus> GetStatistics() const = 0;

    /**
     * @brief Check if asset manager is initialized
     * @return true if initialized, false otherwise
     */
    virtual bool IsInitialized() const = 0;
};

/**
 * @brief Standard font asset manager implementation
 */
class APPLICATION_API FontAssetManager : public IFontAssetManager {
public:
    FontAssetManager();
    ~FontAssetManager() override;

    // Disable copying
    FontAssetManager(const FontAssetManager&) = delete;
    FontAssetManager& operator=(const FontAssetManager&) = delete;

    bool Initialize(const FontAssetManagerConfig& config) override;
    void Shutdown() override;

    AssetLoadResult RegisterAsset(const FontAssetDescriptor& descriptor) override;
    bool UnregisterAsset(uint64_t assetId) override;

    AssetLoadResult LoadAsset(uint64_t assetId) override;
    bool UnloadAsset(uint64_t assetId) override;
    AssetLoadResult ReloadAsset(uint64_t assetId) override;

    FontAssetHandle GetAsset(const std::string& assetName) const override;
    FontAssetHandle GetAsset(uint64_t assetId) const override;
    std::vector<FontAssetHandle> GetAllAssets() const override;

    void SetReloadCallback(AssetReloadCallback callback) override;
    void Update() override;

    size_t ReloadAll() override;
    void ClearCache() override;

    FontAssetManagerConfig GetConfig() const override { return m_Config; }
    bool UpdateConfig(const FontAssetManagerConfig& config) override;

    std::unordered_map<uint64_t, AssetLoadStatus> GetStatistics() const override;
    bool IsInitialized() const override { return m_Initialized; }

private:
    /**
     * @brief Check for file modifications
     */
    void CheckFileModifications();

    /**
     * @brief Get file modification time
     * @param filePath File path
     * @return Modification time, or 0 if error
     */
    uint64_t GetFileModificationTime(const std::string& filePath) const;

    /**
     * @brief Load cached atlas if available
     * @param assetId Asset ID
     * @return true if loaded from cache, false otherwise
     */
    bool LoadCachedAtlas(uint64_t assetId);

    /**
     * @brief Save atlas to cache
     * @param assetId Asset ID
     * @return true if saved, false otherwise
     */
    bool SaveAtlasToCache(uint64_t assetId);

    /**
     * @brief Generate cache file path for an asset
     * @param assetId Asset ID
     * @return Cache file path
     */
    std::string GetCachePath(uint64_t assetId) const;

    /**
     * @brief Generate next asset ID
     * @return New asset ID
     */
    uint64_t GenerateAssetId();

    struct AssetData {
        FontAssetDescriptor descriptor;
        FontAssetHandle handle;
        uint64_t lastModificationTime = 0;
        FT_FaceRec_* face = nullptr;
        std::vector<uint8_t> cachedAtlas;
        bool needsReload = false;
    };

    FontAssetManagerConfig m_Config;
    std::unordered_map<uint64_t, AssetData> m_Assets;
    std::unordered_map<std::string, uint64_t> m_NameToId;
    
    AssetReloadCallback m_ReloadCallback;
    uint64_t m_NextAssetId = 1;
    uint64_t m_LastReloadCheckTime = 0;
    
    mutable std::mutex m_Mutex;
    bool m_Initialized = false;
};

/**
 * @brief Font asset manager utility functions
 */
namespace FontAssetManagerUtils {
    /**
     * @brief Get default asset manager configuration
     */
    FontAssetManagerConfig GetDefaultConfig();

    /**
     * @brief Get development configuration (with hot-reloading)
     */
    FontAssetManagerConfig GetDevelopmentConfig();

    /**
     * @brief Get production configuration (without hot-reloading)
     */
    FontAssetManagerConfig GetProductionConfig();

    /**
     * @brief Validate asset descriptor
     * @param descriptor Descriptor to validate
     * @param outErrors Output error messages
     * @return true if valid, false otherwise
     */
    bool ValidateDescriptor(const FontAssetDescriptor& descriptor, 
                            std::vector<std::string>& outErrors);

    /**
     * @brief Create descriptor from font path
     * @param fontPath Font file path
     * @param assetName Asset name (optional, uses filename if empty)
     * @return FontAssetDescriptor
     */
    FontAssetDescriptor CreateDescriptor(const std::string& fontPath, 
                                         const std::string& assetName = "");
};

} // namespace we::UI::Text
