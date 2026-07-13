#include "Icons/Compile/IconCompiler.h"
#include "Icons/Compile/IconCompileDetail.h"

#include "Icons/Assets/IconAsset.h"
#include "Icons/Core/IconHash.h"
#include "Icons/Core/IconTypes.h"

#include <algorithm>
#include <filesystem>
#include <unordered_map>

namespace we::runtime::icons::compiling {

namespace {

using detail::IsFullColorIcon;
using detail::LoadPngRgbaNative;
using detail::ParseLibGdxAtlasFile;
using detail::ParsedAtlasDescriptor;
using detail::ResolveRuntimeIconName;
using detail::RuntimeAliasesFor;

IconMetaEntry MakeMetaEntry(
    const std::string& name,
    uint32_t tierPx,
    uint32_t atlasWidth,
    uint32_t atlasHeight,
    uint16_t x,
    uint16_t y,
    uint16_t width,
    uint16_t height,
    IconEntryFlags flags)
{
    IconMetaEntry entry;
    entry.name = name;
    entry.nameHash = Fnv1a64(name);
    entry.tierPx = tierPx;
    entry.pixel = {x, y, width, height};
    entry.uv.u0 = static_cast<float>(x) / static_cast<float>(atlasWidth);
    entry.uv.v0 = static_cast<float>(y) / static_cast<float>(atlasHeight);
    entry.uv.u1 = static_cast<float>(x + width) / static_cast<float>(atlasWidth);
    entry.uv.v1 = static_cast<float>(y + height) / static_cast<float>(atlasHeight);
    entry.flags = flags;
    return entry;
}

} // namespace

class DefaultIconCompiler final : public IIconCompiler {
public:
    [[nodiscard]] IconResult<CompileResult> Compile(const CompileOptions& options) const override
    {
        if (options.inputDir.empty() || options.outputDir.empty()) {
            return IconResult<CompileResult>::Failure("Input and output directories are required");
        }
        if (!std::filesystem::is_directory(options.inputDir)) {
            return IconResult<CompileResult>::Failure("Input directory does not exist", options.inputDir.string());
        }

        std::error_code ec;
        std::filesystem::create_directories(options.outputDir, ec);

        CompileResult result;
        IconMetaAsset metaAsset;
        metaAsset.version = assets::kWeIconMetaVersion;

        for (const uint32_t tierPx : kIconAtlasTiers) {
            const auto atlasPath = options.inputDir / ("ui_Atlas_" + std::to_string(tierPx) + ".atlas");
            if (!std::filesystem::exists(atlasPath)) {
                continue;
            }

            ParsedAtlasDescriptor descriptor;
            std::string parseError;
            if (!ParseLibGdxAtlasFile(atlasPath, options.inputDir, descriptor, parseError)) {
                return IconResult<CompileResult>::Failure(parseError, atlasPath.string());
            }

            std::vector<uint8_t> rgba;
            uint32_t pngWidth = 0;
            uint32_t pngHeight = 0;
            if (!LoadPngRgbaNative(descriptor.pngPath, rgba, pngWidth, pngHeight)) {
                return IconResult<CompileResult>::Failure(
                    "Failed to decode atlas PNG",
                    descriptor.pngPath.string());
            }
            if (pngWidth != descriptor.width || pngHeight != descriptor.height) {
                return IconResult<CompileResult>::Failure(
                    "Atlas PNG dimensions do not match descriptor size",
                    descriptor.pngPath.string());
            }

            for (const auto& region : descriptor.regions) {
                const std::string runtimeName = ResolveRuntimeIconName(region.sourceName);
                const auto flags = IsFullColorIcon(runtimeName)
                    ? IconEntryFlags::FullColor
                    : IconEntryFlags::None;

                for (const auto& alias : RuntimeAliasesFor(runtimeName)) {
                    metaAsset.entries.push_back(MakeMetaEntry(
                        alias,
                        tierPx,
                        descriptor.width,
                        descriptor.height,
                        region.x,
                        region.y,
                        region.width,
                        region.height,
                        flags));
                }
            }

            IconAtlasAsset atlasAsset;
            atlasAsset.version = assets::kWeIconAtlasVersion;
            atlasAsset.page.tierPx = tierPx;
            atlasAsset.page.width = descriptor.width;
            atlasAsset.page.height = descriptor.height;
            atlasAsset.page.format = IconAtlasFormat::Rgba8;
            atlasAsset.page.rgba = std::move(rgba);

            const auto validation = assets::IconAssetValidator::Validate(atlasAsset);
            if (!validation.isValid) {
                return IconResult<CompileResult>::Failure(
                    validation.errors.empty() ? "Atlas validation failed" : validation.errors.front(),
                    atlasPath.string());
            }

            const auto outputAtlasPath = options.outputDir / ("ui_Atlas_" + std::to_string(tierPx) + ".weiconatlas");
            const auto writeAtlas = assets::IconAtlasWriter::WriteToFile(atlasAsset, outputAtlasPath);
            if (!writeAtlas.ok) {
                return IconResult<CompileResult>::Failure(writeAtlas.error.message, writeAtlas.error.context);
            }

            result.atlasPaths.push_back(outputAtlasPath);
            ++result.tierCount;
        }

        if (result.atlasPaths.empty()) {
            return IconResult<CompileResult>::Failure(
                "No atlas tiers found in input directory",
                options.inputDir.string());
        }

        const auto metaValidation = assets::IconAssetValidator::Validate(metaAsset);
        if (!metaValidation.isValid) {
            return IconResult<CompileResult>::Failure(
                metaValidation.errors.empty() ? "Meta validation failed" : metaValidation.errors.front());
        }

        result.metaPath = options.outputDir / "icons.weiconmeta";
        const auto writeMeta = assets::IconMetaWriter::WriteToFile(metaAsset, result.metaPath);
        if (!writeMeta.ok) {
            return IconResult<CompileResult>::Failure(writeMeta.error.message, writeMeta.error.context);
        }

        std::unordered_map<std::string, bool> uniqueNames;
        for (const auto& entry : metaAsset.entries) {
            uniqueNames.emplace(entry.name, true);
        }
        result.iconCount = static_cast<uint32_t>(uniqueNames.size());
        return IconResult<CompileResult>::Success(std::move(result));
    }
};

std::unique_ptr<IIconCompiler> CreateIconCompiler()
{
    return std::make_unique<DefaultIconCompiler>();
}

} // namespace we::runtime::icons::compiling
