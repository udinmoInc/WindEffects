#include "Text/Assets/FontAsset.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <unordered_map>

namespace we::runtime::text::assets {

namespace {

enum class ChunkType : uint32_t {
    Metrics = 1,
    Glyphs = 2,
    Kerning = 3,
    Coverage = 4,
    AtlasPages = 5,
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

} // namespace

const GlyphMetrics* FontAsset::FindGlyph(const Codepoint codepoint) const
{
    const auto it = std::lower_bound(
        glyphs.begin(),
        glyphs.end(),
        codepoint,
        [](const GlyphMetrics& glyph, const Codepoint value) {
            return glyph.codepoint < value;
        });
    if (it == glyphs.end() || it->codepoint != codepoint) {
        return nullptr;
    }
    return &(*it);
}

std::unordered_map<Codepoint, size_t> FontAsset::BuildGlyphIndex() const
{
    std::unordered_map<Codepoint, size_t> index;
    index.reserve(glyphs.size());
    for (size_t i = 0; i < glyphs.size(); ++i) {
        index.emplace(glyphs[i].codepoint, i);
    }
    return index;
}

TextResult<FontAsset> FontAssetReader::LoadFromMemory(const std::span<const std::byte> data)
{
    auto input = data;
    uint32_t magic = 0;
    uint16_t version = 0;
    uint8_t atlasFormat = 0;
    uint8_t reserved = 0;
    if (!ReadPod(input, magic) || magic != kWeFontMagic) {
        return TextResult<FontAsset>::Failure("Invalid .wefont magic");
    }
    if (!ReadPod(input, version) || version != kWeFontVersion) {
        return TextResult<FontAsset>::Failure("Unsupported .wefont version");
    }
    if (!ReadPod(input, atlasFormat) || !ReadPod(input, reserved)) {
        return TextResult<FontAsset>::Failure("Truncated .wefont header");
    }

    FontAsset asset;
    asset.atlasFormat = static_cast<AtlasFormat>(atlasFormat);

    while (!input.empty()) {
        ChunkHeader header{};
        if (!ReadPod(input, header.type) || !ReadPod(input, header.size) || input.size() < header.size) {
            return TextResult<FontAsset>::Failure("Truncated .wefont chunk");
        }
        auto chunk = input.subspan(0, header.size);
        input = input.subspan(header.size);

        switch (static_cast<ChunkType>(header.type)) {
        case ChunkType::Metrics: {
            if (!ReadPod(chunk, asset.metrics.ascender)
                || !ReadPod(chunk, asset.metrics.descender)
                || !ReadPod(chunk, asset.metrics.lineHeight)
                || !ReadPod(chunk, asset.metrics.underlinePosition)
                || !ReadPod(chunk, asset.metrics.underlineThickness)
                || !ReadPod(chunk, asset.metrics.bakeSizePx)
                || !ReadPod(chunk, asset.metrics.msdfPixelRange)
                || !ReadPod(chunk, asset.metrics.weight)
                || !ReadString(chunk, asset.metrics.familyName)
                || !ReadString(chunk, asset.metrics.styleName)
                || !ReadPod(chunk, asset.metrics.italic)) {
                return TextResult<FontAsset>::Failure("Invalid metrics chunk");
            }
            break;
        }
        case ChunkType::Glyphs: {
            uint32_t count = 0;
            if (!ReadPod(chunk, count)) {
                return TextResult<FontAsset>::Failure("Invalid glyphs chunk");
            }
            asset.glyphs.resize(count);
            for (auto& glyph : asset.glyphs) {
                uint32_t codepoint = 0;
                if (!ReadPod(chunk, codepoint)
                    || !ReadPod(chunk, glyph.bounds.x)
                    || !ReadPod(chunk, glyph.bounds.y)
                    || !ReadPod(chunk, glyph.bounds.width)
                    || !ReadPod(chunk, glyph.bounds.height)
                    || !ReadPod(chunk, glyph.bearing.x)
                    || !ReadPod(chunk, glyph.bearing.y)
                    || !ReadPod(chunk, glyph.advance)
                    || !ReadPod(chunk, glyph.atlasUv.u0)
                    || !ReadPod(chunk, glyph.atlasUv.v0)
                    || !ReadPod(chunk, glyph.atlasUv.u1)
                    || !ReadPod(chunk, glyph.atlasUv.v1)
                    || !ReadPod(chunk, glyph.atlasPage)
                    || !ReadPod(chunk, glyph.isWhitespace)
                    || !ReadPod(chunk, glyph.hasDrawableQuad)) {
                    return TextResult<FontAsset>::Failure("Invalid glyph entry");
                }
                glyph.codepoint = static_cast<Codepoint>(codepoint);
            }
            break;
        }
        case ChunkType::Kerning: {
            uint32_t count = 0;
            if (!ReadPod(chunk, count)) {
                return TextResult<FontAsset>::Failure("Invalid kerning chunk");
            }
            asset.kerning.resize(count);
            for (auto& pair : asset.kerning) {
                uint32_t left = 0;
                uint32_t right = 0;
                if (!ReadPod(chunk, left) || !ReadPod(chunk, right) || !ReadPod(chunk, pair.advance)) {
                    return TextResult<FontAsset>::Failure("Invalid kerning entry");
                }
                pair.left = static_cast<Codepoint>(left);
                pair.right = static_cast<Codepoint>(right);
            }
            break;
        }
        case ChunkType::Coverage: {
            uint32_t count = 0;
            if (!ReadPod(chunk, count)) {
                return TextResult<FontAsset>::Failure("Invalid coverage chunk");
            }
            asset.coverage.resize(count);
            for (auto& range : asset.coverage) {
                uint32_t start = 0;
                uint32_t end = 0;
                if (!ReadPod(chunk, start) || !ReadPod(chunk, end)) {
                    return TextResult<FontAsset>::Failure("Invalid coverage entry");
                }
                range.start = static_cast<Codepoint>(start);
                range.end = static_cast<Codepoint>(end);
            }
            break;
        }
        case ChunkType::AtlasPages: {
            uint32_t count = 0;
            if (!ReadPod(chunk, count)) {
                return TextResult<FontAsset>::Failure("Invalid atlas chunk");
            }
            asset.atlasPages.resize(count);
            for (auto& page : asset.atlasPages) {
                uint8_t format = 0;
                uint32_t dataSize = 0;
                if (!ReadPod(chunk, page.width)
                    || !ReadPod(chunk, page.height)
                    || !ReadPod(chunk, format)
                    || !ReadPod(chunk, dataSize)
                    || chunk.size() < dataSize) {
                    return TextResult<FontAsset>::Failure("Invalid atlas page");
                }
                page.format = static_cast<AtlasFormat>(format);
                page.rgba.resize(dataSize);
                std::memcpy(page.rgba.data(), chunk.data(), dataSize);
                chunk = chunk.subspan(dataSize);
            }
            break;
        }
        default:
            break;
        }
    }

  std::sort(asset.glyphs.begin(), asset.glyphs.end(), [](const GlyphMetrics& a, const GlyphMetrics& b) {
        return a.codepoint < b.codepoint;
    });

    const auto report = FontAssetValidator::Validate(asset);
    if (!report.isValid) {
        return TextResult<FontAsset>::Failure(report.errors.empty() ? "Invalid font asset" : report.errors.front());
    }

    return TextResult<FontAsset>::Success(std::move(asset));
}

TextResult<FontAsset> FontAssetReader::LoadFromFile(const std::filesystem::path& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return TextResult<FontAsset>::Failure("Failed to open .wefont file", path.string());
    }
    stream.seekg(0, std::ios::end);
    const auto size = stream.tellg();
    stream.seekg(0, std::ios::beg);
    if (size <= 0) {
        return TextResult<FontAsset>::Failure("Empty .wefont file", path.string());
    }

    std::vector<std::byte> bytes(static_cast<size_t>(size));
    stream.read(reinterpret_cast<char*>(bytes.data()), size);
    auto result = LoadFromMemory(bytes);
    if (result.ok) {
        result.value.sourcePath = path;
    }
    return result;
}

TextResult<void> FontAssetWriter::WriteToFile(
    const FontAsset& asset,
    const std::filesystem::path& path)
{
    const auto report = FontAssetValidator::Validate(asset);
    if (!report.isValid) {
        return TextResult<void>::Failure(report.errors.empty() ? "Invalid font asset" : report.errors.front());
    }

    std::vector<std::byte> file;
    file.reserve(1024 * 1024);

    WritePod(file, kWeFontMagic);
    WritePod(file, kWeFontVersion);
    const uint8_t atlasFormat = static_cast<uint8_t>(asset.atlasFormat);
    const uint8_t reserved = 0;
    WritePod(file, atlasFormat);
    WritePod(file, reserved);

    {
        std::vector<std::byte> chunk;
        WritePod(chunk, asset.metrics.ascender);
        WritePod(chunk, asset.metrics.descender);
        WritePod(chunk, asset.metrics.lineHeight);
        WritePod(chunk, asset.metrics.underlinePosition);
        WritePod(chunk, asset.metrics.underlineThickness);
        WritePod(chunk, asset.metrics.bakeSizePx);
        WritePod(chunk, asset.metrics.msdfPixelRange);
        WritePod(chunk, asset.metrics.weight);
        WriteString(chunk, asset.metrics.familyName);
        WriteString(chunk, asset.metrics.styleName);
        WritePod(chunk, asset.metrics.italic);
        WritePod(file, static_cast<uint32_t>(ChunkType::Metrics));
        WritePod(file, static_cast<uint32_t>(chunk.size()));
        file.insert(file.end(), chunk.begin(), chunk.end());
    }

    {
        std::vector<std::byte> chunk;
        const uint32_t count = static_cast<uint32_t>(asset.glyphs.size());
        WritePod(chunk, count);
        for (const auto& glyph : asset.glyphs) {
            const uint32_t codepoint = static_cast<uint32_t>(glyph.codepoint);
            WritePod(chunk, codepoint);
            WritePod(chunk, glyph.bounds.x);
            WritePod(chunk, glyph.bounds.y);
            WritePod(chunk, glyph.bounds.width);
            WritePod(chunk, glyph.bounds.height);
            WritePod(chunk, glyph.bearing.x);
            WritePod(chunk, glyph.bearing.y);
            WritePod(chunk, glyph.advance);
            WritePod(chunk, glyph.atlasUv.u0);
            WritePod(chunk, glyph.atlasUv.v0);
            WritePod(chunk, glyph.atlasUv.u1);
            WritePod(chunk, glyph.atlasUv.v1);
            WritePod(chunk, glyph.atlasPage);
            WritePod(chunk, glyph.isWhitespace);
            WritePod(chunk, glyph.hasDrawableQuad);
        }
        WritePod(file, static_cast<uint32_t>(ChunkType::Glyphs));
        WritePod(file, static_cast<uint32_t>(chunk.size()));
        file.insert(file.end(), chunk.begin(), chunk.end());
    }

    {
        std::vector<std::byte> chunk;
        const uint32_t count = static_cast<uint32_t>(asset.kerning.size());
        WritePod(chunk, count);
        for (const auto& pair : asset.kerning) {
            WritePod(chunk, static_cast<uint32_t>(pair.left));
            WritePod(chunk, static_cast<uint32_t>(pair.right));
            WritePod(chunk, pair.advance);
        }
        WritePod(file, static_cast<uint32_t>(ChunkType::Kerning));
        WritePod(file, static_cast<uint32_t>(chunk.size()));
        file.insert(file.end(), chunk.begin(), chunk.end());
    }

    {
        std::vector<std::byte> chunk;
        const uint32_t count = static_cast<uint32_t>(asset.coverage.size());
        WritePod(chunk, count);
        for (const auto& range : asset.coverage) {
            WritePod(chunk, static_cast<uint32_t>(range.start));
            WritePod(chunk, static_cast<uint32_t>(range.end));
        }
        WritePod(file, static_cast<uint32_t>(ChunkType::Coverage));
        WritePod(file, static_cast<uint32_t>(chunk.size()));
        file.insert(file.end(), chunk.begin(), chunk.end());
    }

    {
        std::vector<std::byte> chunk;
        const uint32_t count = static_cast<uint32_t>(asset.atlasPages.size());
        WritePod(chunk, count);
        for (const auto& page : asset.atlasPages) {
            WritePod(chunk, page.width);
            WritePod(chunk, page.height);
            WritePod(chunk, static_cast<uint8_t>(page.format));
            const uint32_t dataSize = static_cast<uint32_t>(page.rgba.size());
            WritePod(chunk, dataSize);
            const auto* bytes = reinterpret_cast<const std::byte*>(page.rgba.data());
            chunk.insert(chunk.end(), bytes, bytes + page.rgba.size());
        }
        WritePod(file, static_cast<uint32_t>(ChunkType::AtlasPages));
        WritePod(file, static_cast<uint32_t>(chunk.size()));
        file.insert(file.end(), chunk.begin(), chunk.end());
    }

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        return TextResult<void>::Failure("Failed to write .wefont file", path.string());
    }
    stream.write(reinterpret_cast<const char*>(file.data()), static_cast<std::streamsize>(file.size()));
    return TextResult<void>::Success({});
}

FontAssetValidator::ValidationReport FontAssetValidator::Validate(const FontAsset& asset)
{
    ValidationReport report;
    if (asset.metrics.bakeSizePx <= 0.0f) {
        report.isValid = false;
        report.errors.push_back("Bake size must be positive");
    }
    if (asset.metrics.msdfPixelRange <= 0.0f && asset.atlasFormat == AtlasFormat::Msdf) {
        report.isValid = false;
        report.errors.push_back("MSDF pixel range must be positive");
    }
    if (asset.atlasPages.empty()) {
        report.isValid = false;
        report.errors.push_back("Font asset must contain at least one atlas page");
    }

    for (const auto& page : asset.atlasPages) {
        if (page.width == 0 || page.height == 0) {
            report.isValid = false;
            report.errors.push_back("Atlas page dimensions must be non-zero");
        }
        const size_t expected = static_cast<size_t>(page.width) * static_cast<size_t>(page.height) * 4;
        if (page.rgba.size() != expected) {
            report.isValid = false;
            report.errors.push_back("Atlas page pixel data size mismatch");
        }
    }

    for (const auto& glyph : asset.glyphs) {
        if (glyph.atlasUv.u0 < 0.0f || glyph.atlasUv.v0 < 0.0f
            || glyph.atlasUv.u1 > 1.0f || glyph.atlasUv.v1 > 1.0f
            || glyph.atlasUv.u0 > glyph.atlasUv.u1
            || glyph.atlasUv.v0 > glyph.atlasUv.v1) {
            report.isValid = false;
            report.errors.push_back("Glyph UV out of range");
            break;
        }
        if (glyph.atlasPage >= asset.atlasPages.size()) {
            report.isValid = false;
            report.errors.push_back("Glyph references missing atlas page");
            break;
        }
    }

    return report;
}

} // namespace we::runtime::text::assets
