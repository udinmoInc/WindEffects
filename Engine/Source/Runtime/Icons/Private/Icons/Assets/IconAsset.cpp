#include "Icons/Assets/IconAsset.h"

#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <fstream>

namespace we::runtime::icons::assets {

namespace {

enum class ChunkType : uint32_t {
    AtlasPage = 1,
    IconEntries = 1,
};

struct ChunkHeader {
    uint32_t type = 0;
    uint32_t size = 0;
};

template<typename T>
bool ReadPod(std::span<const std::byte>& input, T& out)
{
    if (input.size() < sizeof(T)) {
        return false;
    }
    std::memcpy(&out, input.data(), sizeof(T));
    input = input.subspan(sizeof(T));
    return true;
}

bool ReadString(std::span<const std::byte>& input, std::string& out)
{
    uint32_t length = 0;
    if (!ReadPod(input, length)) {
        return false;
    }
    if (input.size() < length) {
        return false;
    }
    out.assign(reinterpret_cast<const char*>(input.data()), length);
    input = input.subspan(length);
    return true;
}

bool WritePod(std::vector<std::byte>& output, const auto& value)
{
    const auto* bytes = reinterpret_cast<const std::byte*>(&value);
    output.insert(output.end(), bytes, bytes + sizeof(value));
    return true;
}

void WriteString(std::vector<std::byte>& output, const std::string& value)
{
    const uint32_t length = static_cast<uint32_t>(value.size());
    WritePod(output, length);
    const auto* bytes = reinterpret_cast<const std::byte*>(value.data());
    output.insert(output.end(), bytes, bytes + value.size());
}

bool ReadAtlasPage(std::span<const std::byte>& input, IconAtlasPage& page)
{
    if (!ReadPod(input, page.tierPx)
        || !ReadPod(input, page.width)
        || !ReadPod(input, page.height)) {
        return false;
    }

    uint8_t format = 0;
    if (!ReadPod(input, format)) {
        return false;
    }
    page.format = static_cast<IconAtlasFormat>(format);

    uint32_t dataSize = 0;
    if (!ReadPod(input, dataSize) || input.size() < dataSize) {
        return false;
    }
    page.rgba.resize(dataSize);
    std::memcpy(page.rgba.data(), input.data(), dataSize);
    input = input.subspan(dataSize);
    return true;
}

void WriteAtlasPage(std::vector<std::byte>& output, const IconAtlasPage& page)
{
    WritePod(output, page.tierPx);
    WritePod(output, page.width);
    WritePod(output, page.height);
    const uint8_t format = static_cast<uint8_t>(page.format);
    WritePod(output, format);
    const uint32_t dataSize = static_cast<uint32_t>(page.rgba.size());
    WritePod(output, dataSize);
    const auto* bytes = reinterpret_cast<const std::byte*>(page.rgba.data());
    output.insert(output.end(), bytes, bytes + page.rgba.size());
}

bool ReadMetaEntry(std::span<const std::byte>& input, IconMetaEntry& entry)
{
    if (!ReadPod(input, entry.nameHash)
        || !ReadString(input, entry.name)
        || !ReadPod(input, entry.tierPx)) {
        return false;
    }

    if (!ReadPod(input, entry.uv.u0)
        || !ReadPod(input, entry.uv.v0)
        || !ReadPod(input, entry.uv.u1)
        || !ReadPod(input, entry.uv.v1)) {
        return false;
    }

    if (!ReadPod(input, entry.pixel.x)
        || !ReadPod(input, entry.pixel.y)
        || !ReadPod(input, entry.pixel.width)
        || !ReadPod(input, entry.pixel.height)) {
        return false;
    }

    uint8_t flags = 0;
    if (!ReadPod(input, flags)) {
        return false;
    }
    entry.flags = static_cast<IconEntryFlags>(flags);
    return true;
}

void WriteMetaEntry(std::vector<std::byte>& output, const IconMetaEntry& entry)
{
    WritePod(output, entry.nameHash);
    WriteString(output, entry.name);
    WritePod(output, entry.tierPx);
    WritePod(output, entry.uv.u0);
    WritePod(output, entry.uv.v0);
    WritePod(output, entry.uv.u1);
    WritePod(output, entry.uv.v1);
    WritePod(output, entry.pixel.x);
    WritePod(output, entry.pixel.y);
    WritePod(output, entry.pixel.width);
    WritePod(output, entry.pixel.height);
    const uint8_t flags = static_cast<uint8_t>(entry.flags);
    WritePod(output, flags);
}

std::vector<std::byte> ReadFileBytes(const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    input.seekg(0, std::ios::beg);
    std::vector<std::byte> bytes(static_cast<size_t>(size));
    input.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

} // namespace

IconResult<IconAtlasAsset> IconAtlasReader::LoadFromMemory(const std::span<const std::byte> data)
{
    auto input = data;
    uint32_t magic = 0;
    uint16_t version = 0;
    if (!ReadPod(input, magic) || magic != kWeIconAtlasMagic) {
        return IconResult<IconAtlasAsset>::Failure("Invalid .weiconatlas magic");
    }
    if (!ReadPod(input, version) || version != kWeIconAtlasVersion) {
        return IconResult<IconAtlasAsset>::Failure("Unsupported .weiconatlas version");
    }

    IconAtlasAsset asset;
    asset.version = version;

    while (!input.empty()) {
        ChunkHeader header{};
        if (!ReadPod(input, header.type) || !ReadPod(input, header.size) || input.size() < header.size) {
            return IconResult<IconAtlasAsset>::Failure("Corrupt .weiconatlas chunk header");
        }

        auto chunk = input.subspan(0, header.size);
        input = input.subspan(header.size);

        if (header.type == static_cast<uint32_t>(ChunkType::AtlasPage)) {
            if (!ReadAtlasPage(chunk, asset.page)) {
                return IconResult<IconAtlasAsset>::Failure("Failed to read atlas page chunk");
            }
        }
    }

    return IconResult<IconAtlasAsset>::Success(std::move(asset));
}

IconResult<IconAtlasAsset> IconAtlasReader::LoadFromFile(const std::filesystem::path& path)
{
    const auto bytes = ReadFileBytes(path);
    if (bytes.empty()) {
        return IconResult<IconAtlasAsset>::Failure("Failed to read .weiconatlas file", path.string());
    }
    return LoadFromMemory(bytes);
}

bool LoadAtlasPixels(
    const char* pathUtf8,
    uint32_t* outWidth,
    uint32_t* outHeight,
    uint32_t* outTierPx,
    uint8_t** outRgba,
    uint32_t* outRgbaSize,
    char* errorBuf,
    size_t errorBufSize)
{
    if (outWidth) {
        *outWidth = 0;
    }
    if (outHeight) {
        *outHeight = 0;
    }
    if (outTierPx) {
        *outTierPx = 0;
    }
    if (outRgba) {
        *outRgba = nullptr;
    }
    if (outRgbaSize) {
        *outRgbaSize = 0;
    }

    const auto writeError = [&](const std::string& message) {
        if (errorBuf && errorBufSize > 0) {
            const size_t n = std::min(message.size(), errorBufSize - 1u);
            if (n > 0) {
                std::memcpy(errorBuf, message.data(), n);
            }
            errorBuf[n] = '\0';
        }
    };

    if (!pathUtf8 || pathUtf8[0] == '\0' || !outWidth || !outHeight || !outRgba || !outRgbaSize) {
        writeError("Invalid LoadAtlasPixels arguments");
        return false;
    }

    const auto loaded = IconAtlasReader::LoadFromFile(std::filesystem::path(pathUtf8));
    if (!loaded.ok) {
        writeError(loaded.error.message.empty() ? "Failed to load icon atlas" : loaded.error.message);
        return false;
    }

    const auto& page = loaded.value.page;
    if (page.width == 0 || page.height == 0 || page.rgba.empty()) {
        writeError("Atlas page is empty");
        return false;
    }

    auto* rgba = static_cast<uint8_t*>(std::malloc(page.rgba.size()));
    if (!rgba) {
        writeError("Out of memory loading icon atlas");
        return false;
    }
    std::memcpy(rgba, page.rgba.data(), page.rgba.size());
    *outWidth = page.width;
    *outHeight = page.height;
    if (outTierPx) {
        *outTierPx = page.tierPx;
    }
    *outRgba = rgba;
    *outRgbaSize = static_cast<uint32_t>(page.rgba.size());
    if (errorBuf && errorBufSize > 0) {
        errorBuf[0] = '\0';
    }
    return true;
}

void FreeAtlasPixels(uint8_t* rgba)
{
    std::free(rgba);
}

IconResult<void> IconAtlasWriter::WriteToFile(const IconAtlasAsset& asset, const std::filesystem::path& path)
{
    std::vector<std::byte> output;
    WritePod(output, kWeIconAtlasMagic);
    WritePod(output, kWeIconAtlasVersion);

    std::vector<std::byte> pageChunk;
    WriteAtlasPage(pageChunk, asset.page);

    const uint32_t chunkType = static_cast<uint32_t>(ChunkType::AtlasPage);
    const uint32_t chunkSize = static_cast<uint32_t>(pageChunk.size());
    WritePod(output, chunkType);
    WritePod(output, chunkSize);
    output.insert(output.end(), pageChunk.begin(), pageChunk.end());

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return IconResult<void>::Failure("Failed to open .weiconatlas for writing", path.string());
    }
    file.write(reinterpret_cast<const char*>(output.data()), static_cast<std::streamsize>(output.size()));
    return IconResult<void>::Success();
}

IconResult<IconMetaAsset> IconMetaReader::LoadFromMemory(const std::span<const std::byte> data)
{
    auto input = data;
    uint32_t magic = 0;
    uint16_t version = 0;
    if (!ReadPod(input, magic) || magic != kWeIconMetaMagic) {
        return IconResult<IconMetaAsset>::Failure("Invalid .weiconmeta magic");
    }
    if (!ReadPod(input, version) || version != kWeIconMetaVersion) {
        return IconResult<IconMetaAsset>::Failure("Unsupported .weiconmeta version");
    }

    IconMetaAsset asset;
    asset.version = version;

    while (!input.empty()) {
        ChunkHeader header{};
        if (!ReadPod(input, header.type) || !ReadPod(input, header.size) || input.size() < header.size) {
            return IconResult<IconMetaAsset>::Failure("Corrupt .weiconmeta chunk header");
        }

        auto chunk = input.subspan(0, header.size);
        input = input.subspan(header.size);

        if (header.type == static_cast<uint32_t>(ChunkType::IconEntries)) {
            uint32_t count = 0;
            if (!ReadPod(chunk, count)) {
                return IconResult<IconMetaAsset>::Failure("Failed to read icon entry count");
            }
            asset.entries.reserve(count);
            for (uint32_t i = 0; i < count; ++i) {
                IconMetaEntry entry;
                if (!ReadMetaEntry(chunk, entry)) {
                    return IconResult<IconMetaAsset>::Failure("Failed to read icon meta entry");
                }
                asset.entries.push_back(std::move(entry));
            }
        }
    }

    return IconResult<IconMetaAsset>::Success(std::move(asset));
}

IconResult<IconMetaAsset> IconMetaReader::LoadFromFile(const std::filesystem::path& path)
{
    const auto bytes = ReadFileBytes(path);
    if (bytes.empty()) {
        return IconResult<IconMetaAsset>::Failure("Failed to read .weiconmeta file", path.string());
    }
    return LoadFromMemory(bytes);
}

bool LoadFlatList(
    const char* pathUtf8,
    IconMetaFlatList& outList,
    char* errorBuf,
    size_t errorBufSize)
{
    FreeFlatList(outList);
    const auto writeError = [&](const std::string& message) {
        if (errorBuf && errorBufSize > 0) {
            const size_t n = std::min(message.size(), errorBufSize - 1u);
            if (n > 0) {
                std::memcpy(errorBuf, message.data(), n);
            }
            errorBuf[n] = '\0';
        }
    };

    if (!pathUtf8 || pathUtf8[0] == '\0') {
        writeError("Empty icon meta path");
        return false;
    }

    const auto loaded = IconMetaReader::LoadFromFile(std::filesystem::path(pathUtf8));
    if (!loaded.ok) {
        writeError(loaded.error.message.empty() ? "Failed to load icon meta" : loaded.error.message);
        return false;
    }

    const uint32_t count = static_cast<uint32_t>(loaded.value.entries.size());
    if (count == 0) {
        if (errorBuf && errorBufSize > 0) {
            errorBuf[0] = '\0';
        }
        return true;
    }

    auto* entries = static_cast<IconMetaFlatEntry*>(std::malloc(sizeof(IconMetaFlatEntry) * count));
    if (!entries) {
        writeError("Out of memory loading icon meta");
        return false;
    }

    for (uint32_t i = 0; i < count; ++i) {
        const auto& src = loaded.value.entries[i];
        IconMetaFlatEntry& dst = entries[i];
        dst = {};
        dst.nameHash = src.nameHash;
        dst.tierPx = src.tierPx;
        dst.u0 = src.uv.u0;
        dst.v0 = src.uv.v0;
        dst.u1 = src.uv.u1;
        dst.v1 = src.uv.v1;
        dst.flags = static_cast<uint8_t>(src.flags);
        const size_t copyLen = std::min(src.name.size(), sizeof(dst.name) - 1u);
        if (copyLen > 0) {
            std::memcpy(dst.name, src.name.data(), copyLen);
        }
        dst.name[copyLen] = '\0';
    }

    outList.entries = entries;
    outList.count = count;
    if (errorBuf && errorBufSize > 0) {
        errorBuf[0] = '\0';
    }
    return true;
}

void FreeFlatList(IconMetaFlatList& list)
{
    if (list.entries) {
        std::free(list.entries);
        list.entries = nullptr;
    }
    list.count = 0;
}

IconResult<void> IconMetaWriter::WriteToFile(const IconMetaAsset& asset, const std::filesystem::path& path)
{
    std::vector<std::byte> output;
    WritePod(output, kWeIconMetaMagic);
    WritePod(output, kWeIconMetaVersion);

    std::vector<std::byte> entriesChunk;
    const uint32_t count = static_cast<uint32_t>(asset.entries.size());
    WritePod(entriesChunk, count);
    for (const auto& entry : asset.entries) {
        WriteMetaEntry(entriesChunk, entry);
    }

    const uint32_t chunkType = static_cast<uint32_t>(ChunkType::IconEntries);
    const uint32_t chunkSize = static_cast<uint32_t>(entriesChunk.size());
    WritePod(output, chunkType);
    WritePod(output, chunkSize);
    output.insert(output.end(), entriesChunk.begin(), entriesChunk.end());

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        return IconResult<void>::Failure("Failed to open .weiconmeta for writing", path.string());
    }
    file.write(reinterpret_cast<const char*>(output.data()), static_cast<std::streamsize>(output.size()));
    return IconResult<void>::Success();
}

IconAssetValidator::ValidationReport IconAssetValidator::Validate(const IconAtlasAsset& asset)
{
    ValidationReport report;
    const auto& page = asset.page;
    if (page.width == 0 || page.height == 0) {
        report.isValid = false;
        report.errors.push_back("Atlas page has zero dimensions");
    }
    if (page.tierPx == 0) {
        report.isValid = false;
        report.errors.push_back("Atlas tier is zero");
    }
    const size_t expected = static_cast<size_t>(page.width) * static_cast<size_t>(page.height) * 4u;
    if (page.rgba.size() != expected) {
        report.isValid = false;
        report.errors.push_back("Atlas RGBA size does not match width*height*4");
    }
    return report;
}

IconAssetValidator::ValidationReport IconAssetValidator::Validate(const IconMetaAsset& asset)
{
    ValidationReport report;
    if (asset.entries.empty()) {
        report.warnings.push_back("Icon meta contains no entries");
    }
    for (const auto& entry : asset.entries) {
        if (entry.name.empty()) {
            report.isValid = false;
            report.errors.push_back("Icon meta entry has empty name");
        }
        if (entry.uv.u1 <= entry.uv.u0 || entry.uv.v1 <= entry.uv.v0) {
            report.isValid = false;
            report.errors.push_back("Icon '" + entry.name + "' has invalid UV rect");
        }
    }
    return report;
}

} // namespace we::runtime::icons::assets
