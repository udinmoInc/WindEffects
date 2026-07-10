#pragma once

#include "Text/Assets/FontAsset.h"
#include "Text/Core/Types.h"
#include "Text/Export.h"

#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::text::assets {

struct FontStackEntry {
    std::string family;
    std::string assetPath;
    std::vector<std::string> scripts;
};

struct FontStackConfig {
    std::string defaultFamily;
    std::vector<FontStackEntry> fallbackChain;
};

class TEXT_API IFontAssetManager {
public:
    virtual ~IFontAssetManager() = default;

    [[nodiscard]] virtual TextResult<FontHandle> LoadSync(const std::filesystem::path& assetPath) = 0;
    [[nodiscard]] virtual std::future<TextResult<FontHandle>> LoadAsync(const std::filesystem::path& assetPath) = 0;
    [[nodiscard]] virtual std::shared_ptr<const FontAsset> GetAsset(FontHandle handle) const = 0;
    virtual void Unload(FontHandle handle) = 0;
    virtual bool Reload(FontHandle handle) = 0;
    [[nodiscard]] virtual std::vector<FontHandle> GetLoadedFonts() const = 0;
    [[nodiscard]] virtual bool LoadFontStackConfig(const std::filesystem::path& configPath) = 0;
    [[nodiscard]] virtual const FontStackConfig& GetFontStackConfig() const = 0;
};

class TEXT_API FontAssetManager final : public IFontAssetManager {
public:
    FontAssetManager();
    ~FontAssetManager() override;

    [[nodiscard]] TextResult<FontHandle> LoadSync(const std::filesystem::path& assetPath) override;
    [[nodiscard]] std::future<TextResult<FontHandle>> LoadAsync(const std::filesystem::path& assetPath) override;
    [[nodiscard]] std::shared_ptr<const FontAsset> GetAsset(FontHandle handle) const override;
    void Unload(FontHandle handle) override;
    bool Reload(FontHandle handle) override;
    [[nodiscard]] std::vector<FontHandle> GetLoadedFonts() const override;
    [[nodiscard]] bool LoadFontStackConfig(const std::filesystem::path& configPath) override;
    [[nodiscard]] const FontStackConfig& GetFontStackConfig() const override { return m_StackConfig; }

private:
    struct LoadedFont {
        std::filesystem::path path;
        std::shared_ptr<const FontAsset> asset;
    };

    [[nodiscard]] TextResult<FontHandle> LoadInternal(const std::filesystem::path& assetPath);

    mutable std::mutex m_Mutex;
    FontHandle m_NextHandle = 1;
    std::unordered_map<FontHandle, LoadedFont> m_Fonts;
    FontStackConfig m_StackConfig;
};

[[nodiscard]] TEXT_API std::unique_ptr<IFontAssetManager> CreateFontAssetManager();

} // namespace we::runtime::text::assets
