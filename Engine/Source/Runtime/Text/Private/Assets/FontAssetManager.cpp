#include "Text/Assets/FontAssetManager.h"

#include "Core/Logger.h"

#include <fstream>
#include <sstream>

namespace we::runtime::text::assets {

namespace {

bool ParseStringField(const std::string& json, const std::string& key, std::string& outValue)
{
    const std::string token = "\"" + key + "\"";
    const size_t keyPos = json.find(token);
    if (keyPos == std::string::npos) {
        return false;
    }
    const size_t colon = json.find(':', keyPos + token.size());
    const size_t start = json.find('"', colon);
    const size_t end = json.find('"', start + 1);
    if (start == std::string::npos || end == std::string::npos) {
        return false;
    }
    outValue = json.substr(start + 1, end - start - 1);
    return true;
}

} // namespace

FontAssetManager::FontAssetManager() = default;
FontAssetManager::~FontAssetManager() = default;

TextResult<FontHandle> FontAssetManager::LoadInternal(const std::filesystem::path& assetPath)
{
    auto loaded = FontAssetReader::LoadFromFile(assetPath);
    if (!loaded.ok) {
        return TextResult<FontHandle>::Failure(loaded.error.message, loaded.error.context);
    }

    const FontHandle handle = m_NextHandle++;
    m_Fonts.emplace(handle, LoadedFont{assetPath, std::make_shared<const FontAsset>(std::move(loaded.value))});
    return TextResult<FontHandle>::Success(handle);
}

TextResult<FontHandle> FontAssetManager::LoadSync(const std::filesystem::path& assetPath)
{
    std::lock_guard lock(m_Mutex);
    for (const auto& [handle, font] : m_Fonts) {
        if (font.path == assetPath) {
            return TextResult<FontHandle>::Success(handle);
        }
    }
    return LoadInternal(assetPath);
}

std::future<TextResult<FontHandle>> FontAssetManager::LoadAsync(const std::filesystem::path& assetPath)
{
    return std::async(std::launch::async, [this, assetPath]() {
        return LoadSync(assetPath);
    });
}

std::shared_ptr<const FontAsset> FontAssetManager::GetAsset(const FontHandle handle) const
{
    std::lock_guard lock(m_Mutex);
    const auto it = m_Fonts.find(handle);
    if (it == m_Fonts.end()) {
        return nullptr;
    }
    return it->second.asset;
}

void FontAssetManager::Unload(const FontHandle handle)
{
    std::lock_guard lock(m_Mutex);
    m_Fonts.erase(handle);
}

bool FontAssetManager::Reload(const FontHandle handle)
{
    std::lock_guard lock(m_Mutex);
    const auto it = m_Fonts.find(handle);
    if (it == m_Fonts.end()) {
        return false;
    }
    const auto path = it->second.path;
    auto loaded = FontAssetReader::LoadFromFile(path);
    if (!loaded.ok) {
        WE_LOG_ERROR("Text", "Failed to reload font: " + loaded.error.message);
        return false;
    }
    it->second.asset = std::make_shared<const FontAsset>(std::move(loaded.value));
    return true;
}

std::vector<FontHandle> FontAssetManager::GetLoadedFonts() const
{
    std::lock_guard lock(m_Mutex);
    std::vector<FontHandle> handles;
    handles.reserve(m_Fonts.size());
    for (const auto& [handle, _] : m_Fonts) {
        handles.push_back(handle);
    }
    return handles;
}

bool FontAssetManager::LoadFontStackConfig(const std::filesystem::path& configPath)
{
    std::ifstream stream(configPath);
    if (!stream) {
        WE_LOG_ERROR("Text", "Failed to open font stack config: " + configPath.string());
        return false;
    }

    std::stringstream buffer;
    buffer << stream.rdbuf();
    const std::string json = buffer.str();

    FontStackConfig config;
    ParseStringField(json, "defaultFamily", config.defaultFamily);

    size_t searchPos = 0;
    while (true) {
        const size_t familyPos = json.find("\"family\"", searchPos);
        if (familyPos == std::string::npos) {
            break;
        }
        FontStackEntry entry;
        const size_t colon = json.find(':', familyPos);
        const size_t start = json.find('"', colon);
        const size_t end = json.find('"', start + 1);
        if (start == std::string::npos || end == std::string::npos) {
            break;
        }
        entry.family = json.substr(start + 1, end - start - 1);
        config.fallbackChain.push_back(std::move(entry));
        searchPos = end + 1;
    }

    m_StackConfig = std::move(config);
    return true;
}

std::unique_ptr<IFontAssetManager> CreateFontAssetManager()
{
    return std::make_unique<FontAssetManager>();
}

} // namespace we::runtime::text::assets
