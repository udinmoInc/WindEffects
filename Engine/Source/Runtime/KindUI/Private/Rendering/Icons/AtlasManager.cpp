#include "KindUI/Rendering/Icons/AtlasManager.h"

#include "KindUI/Rendering/OverlayRenderer.h"
#include "Core/Logger.h"

#include <cstring>
#include <fstream>
#include <string>
#include <vector>

namespace we::runtime::kindui {
namespace {

constexpr uint32_t kWeIconAtlasMagic = 0x534F4349; // "ICOS"
constexpr uint16_t kWeIconAtlasVersion = 1;

template<typename T>
bool ReadPod(const std::vector<uint8_t>& bytes, size_t& offset, T& out) {
    if (offset + sizeof(T) > bytes.size()) {
        return false;
    }
    std::memcpy(&out, bytes.data() + offset, sizeof(T));
    offset += sizeof(T);
    return true;
}

bool LoadWeIconAtlasFile(
    const std::filesystem::path& path,
    uint32_t& outWidth,
    uint32_t& outHeight,
    uint32_t& outTierPx,
    std::vector<uint8_t>& outRgba,
    std::string& outError)
{
    outWidth = 0;
    outHeight = 0;
    outTierPx = 0;
    outRgba.clear();

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        outError = "Failed to open atlas file";
        return false;
    }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0, std::ios::beg);
    if (size <= 0) {
        outError = "Empty atlas file";
        return false;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(size));
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    if (!input) {
        outError = "Failed to read atlas file";
        return false;
    }

    size_t offset = 0;
    uint32_t magic = 0;
    uint16_t version = 0;
    if (!ReadPod(bytes, offset, magic) || magic != kWeIconAtlasMagic) {
        outError = "Invalid .weiconatlas magic";
        return false;
    }
    if (!ReadPod(bytes, offset, version) || version != kWeIconAtlasVersion) {
        outError = "Unsupported .weiconatlas version";
        return false;
    }

    bool foundPage = false;
    while (offset + 8 <= bytes.size()) {
        uint32_t chunkType = 0;
        uint32_t chunkSize = 0;
        if (!ReadPod(bytes, offset, chunkType) || !ReadPod(bytes, offset, chunkSize)) {
            outError = "Corrupt atlas chunk header";
            return false;
        }
        if (chunkSize > bytes.size() - offset) {
            outError = "Corrupt atlas chunk size";
            return false;
        }
        const size_t chunkStart = offset;
        const size_t chunkEnd = offset + chunkSize;
        offset = chunkEnd;

        if (chunkType != 1) {
            continue;
        }

        size_t cursor = chunkStart;
        uint32_t tierPx = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint8_t format = 0;
        uint32_t dataSize = 0;
        if (!ReadPod(bytes, cursor, tierPx)
            || !ReadPod(bytes, cursor, width)
            || !ReadPod(bytes, cursor, height)
            || !ReadPod(bytes, cursor, format)
            || !ReadPod(bytes, cursor, dataSize)) {
            outError = "Corrupt atlas page header";
            return false;
        }
        if (width == 0 || height == 0 || dataSize == 0 || cursor + dataSize > chunkEnd) {
            outError = "Corrupt atlas page pixels";
            return false;
        }
        if (dataSize != width * height * 4u) {
            outError = "Atlas RGBA size mismatch";
            return false;
        }

        outTierPx = tierPx;
        outWidth = width;
        outHeight = height;
        outRgba.resize(dataSize);
        std::memcpy(outRgba.data(), bytes.data() + cursor, dataSize);
        foundPage = true;
        break;
    }

    if (!foundPage) {
        outError = "Atlas page chunk not found";
        return false;
    }
    return true;
}

} // namespace

AtlasManager::~AtlasManager() {
    Shutdown();
}

bool AtlasManager::Init(OverlayRenderer* renderer) {
    m_Renderer = renderer;
    m_Tiers.reserve(32);
    m_TierPaths.reserve(32);
    return m_Renderer != nullptr;
}

void AtlasManager::Shutdown() {
    std::scoped_lock lock(m_Mutex);
    for (auto& tier : m_Tiers) {
        DestroyTier(tier);
    }
    m_Tiers.clear();
    m_TierPaths.clear();
    m_Renderer = nullptr;
}

int AtlasManager::FindTierIndex(uint32_t tierPx) const {
    for (size_t i = 0; i < m_Tiers.size(); ++i) {
        if (m_Tiers[i].tierPx == tierPx) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool AtlasManager::LoadTierFromFile(uint32_t tierPx, const std::filesystem::path& atlasPath) {
    if (tierPx == 0 || !m_Renderer) {
        return false;
    }

    HE_INFO("[AtlasManager] Loading atlas file: " + atlasPath.string());

    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t fileTier = 0;
    std::vector<uint8_t> rgba;
    std::string error;
    if (!LoadWeIconAtlasFile(atlasPath, width, height, fileTier, rgba, error)) {
        HE_ERROR("[AtlasManager] Failed to load atlas: " + atlasPath.string() + " (" + error + ")");
        return false;
    }

    HE_INFO(
        "[AtlasManager] Loaded atlas pixels: " + std::to_string(width) + "x"
        + std::to_string(height) + " (" + std::to_string(rgba.size()) + " bytes)");
    HE_INFO("[AtlasManager] Uploading atlas to GPU...");

    AtlasGpuResource uploaded{};
    uploaded.tierPx = tierPx;
    if (!UploadAtlasPage(uploaded, rgba, width, height)) {
        HE_ERROR("[AtlasManager] Atlas tier upload failed: " + std::to_string(tierPx));
        return false;
    }

    {
        std::scoped_lock lock(m_Mutex);
        int index = FindTierIndex(tierPx);
        if (index < 0) {
            m_Tiers.push_back(AtlasGpuResource{});
            m_TierPaths.emplace_back();
            index = static_cast<int>(m_Tiers.size() - 1);
        } else {
            DestroyTier(m_Tiers[static_cast<size_t>(index)]);
        }
        m_TierPaths[static_cast<size_t>(index)] = atlasPath;
        m_Tiers[static_cast<size_t>(index)] = uploaded;
        m_Tiers[static_cast<size_t>(index)].ready = true;
    }

    HE_INFO("[AtlasManager] Atlas tier uploaded: " + std::to_string(tierPx));
    return true;
}

bool AtlasManager::EnsureTierLoaded(uint32_t tierPx, const std::filesystem::path& atlasPath) {
    if (tierPx == 0) {
        return false;
    }
    {
        std::scoped_lock lock(m_Mutex);
        const int index = FindTierIndex(tierPx);
        if (index >= 0 && m_Tiers[static_cast<size_t>(index)].ready) {
            return true;
        }
    }
    return LoadTierFromFile(tierPx, atlasPath);
}

const AtlasGpuResource* AtlasManager::GetTier(uint32_t tierPx) const {
    std::scoped_lock lock(m_Mutex);
    const int index = FindTierIndex(tierPx);
    if (index < 0 || !m_Tiers[static_cast<size_t>(index)].ready) {
        return nullptr;
    }
    return &m_Tiers[static_cast<size_t>(index)];
}

bool AtlasManager::IsTierReady(uint32_t tierPx) const {
    return GetTier(tierPx) != nullptr;
}

void AtlasManager::WaitDeviceIdle() const {
}

bool AtlasManager::UploadAtlasPage(
    AtlasGpuResource& tier, const std::vector<uint8_t>& rgba, uint32_t width, uint32_t height) {
    if (!m_Renderer || rgba.empty() || width == 0 || height == 0) {
        return false;
    }
    tier.width = width;
    tier.height = height;
    tier.descriptorSet = m_Renderer->UploadRgbaTexture(width, height, rgba, false);
    return tier.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid;
}

void AtlasManager::DestroyTier(AtlasGpuResource& tier) {
    if (m_Renderer && tier.descriptorSet != we::rhi::RHIDescriptorSetHandle::Invalid) {
        m_Renderer->UnregisterTexture(tier.descriptorSet);
    }
    tier = {};
}

} // namespace we::runtime::kindui
