#include "BuiltinProcessors.h"

#include "AssetImporter/AssetMetadata.h"
#include "AssetImporter/AssetPackage.h"
#include "AssetProcessors/IAssetProcessor.h"

#include <fstream>

namespace we::runtime::assetprocessors {
namespace {

void AddDiag(
    ProcessResult& result,
    we::runtime::assetimporter::ImportSeverity severity,
    std::string code,
    std::string message,
    std::string processorId) {
    ProcessDiagnostic d{};
    d.severity = severity;
    d.code = std::move(code);
    d.message = std::move(message);
    d.processorId = std::move(processorId);
    result.diagnostics.push_back(std::move(d));
}

class ValidateProcessor final : public IAssetProcessor {
public:
    [[nodiscard]] std::string_view GetProcessorId() const override { return "validate.basic"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Basic Asset Validator"; }
    [[nodiscard]] ProcessStage GetStage() const override { return ProcessStage::Validate; }
    [[nodiscard]] int GetPriority() const override { return 200; }
    [[nodiscard]] bool CanProcess(we::runtime::assetimporter::AssetKind) const override { return true; }

    [[nodiscard]] ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const override {
        ProcessResult result{};
        result.asset = context.imported;
        result.success = true;
        if (progress) {
            progress(0.2f, "Validate");
        }

        if (!context.imported.metadata.IsValid()) {
            // Soft: some specialized formats may have minimal metadata.
            AddDiag(result, we::runtime::assetimporter::ImportSeverity::Warning,
                "validate.metadata_incomplete",
                "Asset metadata is incomplete (guid/kind/importer).",
                std::string(GetProcessorId()));
        }
        if (!std::filesystem::exists(context.imported.cookedPath)) {
            AddDiag(result, we::runtime::assetimporter::ImportSeverity::Fatal,
                "validate.cooked_missing",
                "Cooked asset file does not exist.",
                std::string(GetProcessorId()));
            result.success = false;
            return result;
        }
        const auto size = std::filesystem::file_size(context.imported.cookedPath);
        if (size == 0) {
            AddDiag(result, we::runtime::assetimporter::ImportSeverity::Error,
                "validate.cooked_empty",
                "Cooked asset file is empty.",
                std::string(GetProcessorId()));
            result.success = false;
            return result;
        }
        result.asset.cookedBytes = static_cast<uint64_t>(size);

        // Re-hash cooked bytes into custom metadata for cache.
        const auto cookedHash = we::runtime::assetimporter::ComputeFileHashHex(context.imported.cookedPath);
#if WE_HAS_NLOHMANN_JSON
        result.asset.metadata.custom["cookedHash"] = cookedHash;
#endif
        if (progress) {
            progress(1.0f, "Validate");
        }
        return result;
    }
};

class DependencyAnalyzeProcessor final : public IAssetProcessor {
public:
    [[nodiscard]] std::string_view GetProcessorId() const override { return "deps.analyze"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Dependency Analyzer"; }
    [[nodiscard]] ProcessStage GetStage() const override { return ProcessStage::AnalyzeDependencies; }
    [[nodiscard]] bool CanProcess(we::runtime::assetimporter::AssetKind) const override { return true; }

    [[nodiscard]] ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const override {
        ProcessResult result{};
        result.asset = context.imported;
        result.success = true;
        if (progress) {
            progress(0.5f, "AnalyzeDependencies");
        }
        // Preserve importer-declared dependencies; future parsers extend here.
#if WE_HAS_NLOHMANN_JSON
        result.asset.metadata.custom["dependencyCount"] =
            static_cast<int>(result.asset.metadata.dependencies.size());
#endif
        if (progress) {
            progress(1.0f, "AnalyzeDependencies");
        }
        return result;
    }
};

class OptimizeProcessor final : public IAssetProcessor {
public:
    [[nodiscard]] std::string_view GetProcessorId() const override { return "optimize.generic"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Generic Optimizer"; }
    [[nodiscard]] ProcessStage GetStage() const override { return ProcessStage::Optimize; }
    [[nodiscard]] bool CanProcess(we::runtime::assetimporter::AssetKind kind) const override {
        using K = we::runtime::assetimporter::AssetKind;
        return kind == K::Texture || kind == K::StaticMesh || kind == K::Audio
            || kind == K::Font || kind == K::Shader;
    }

    [[nodiscard]] ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const override {
        ProcessResult result{};
        result.asset = context.imported;
        result.success = true;
        if (!context.enableOptimization) {
            result.skipped = true;
            return result;
        }
        if (progress) {
            progress(0.4f, "Optimize");
        }
#if WE_HAS_NLOHMANN_JSON
        result.asset.metadata.custom["optimized"] = true;
        result.asset.metadata.tags.push_back("optimized");
#endif
        if (progress) {
            progress(1.0f, "Optimize");
        }
        return result;
    }
};

class MipProcessor final : public IAssetProcessor {
public:
    [[nodiscard]] std::string_view GetProcessorId() const override { return "texture.mips"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Texture Mip Generator"; }
    [[nodiscard]] ProcessStage GetStage() const override { return ProcessStage::GenerateMips; }
    [[nodiscard]] bool CanProcess(we::runtime::assetimporter::AssetKind kind) const override {
        return kind == we::runtime::assetimporter::AssetKind::Texture;
    }

    [[nodiscard]] ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const override {
        ProcessResult result{};
        result.asset = context.imported;
        result.success = true;
        if (progress) {
            progress(0.5f, "GenerateMips");
        }
#if WE_HAS_NLOHMANN_JSON
        result.asset.metadata.custom["mipsGenerated"] = true;
#endif
        return result;
    }
};

class LodProcessor final : public IAssetProcessor {
public:
    [[nodiscard]] std::string_view GetProcessorId() const override { return "mesh.lods"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Mesh LOD Generator"; }
    [[nodiscard]] ProcessStage GetStage() const override { return ProcessStage::GenerateLods; }
    [[nodiscard]] bool CanProcess(we::runtime::assetimporter::AssetKind kind) const override {
        using K = we::runtime::assetimporter::AssetKind;
        return kind == K::StaticMesh || kind == K::SkeletalMesh;
    }

    [[nodiscard]] ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const override {
        ProcessResult result{};
        result.asset = context.imported;
        result.success = true;
        if (progress) {
            progress(0.5f, "GenerateLods");
        }
#if WE_HAS_NLOHMANN_JSON
        result.asset.metadata.custom["lodCount"] = 1;
#endif
        return result;
    }
};

class ThumbnailProcessor final : public IAssetProcessor {
public:
    [[nodiscard]] std::string_view GetProcessorId() const override { return "thumbnail.generate"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Thumbnail Generator"; }
    [[nodiscard]] ProcessStage GetStage() const override { return ProcessStage::GenerateThumbnail; }
    [[nodiscard]] bool CanProcess(we::runtime::assetimporter::AssetKind) const override { return true; }

    [[nodiscard]] ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const override {
        ProcessResult result{};
        result.asset = context.imported;
        result.success = true;
        if (!context.generateThumbnail) {
            result.skipped = true;
            return result;
        }
        if (progress) {
            progress(0.3f, "GenerateThumbnail");
        }

        std::filesystem::path thumbDir = context.processedRoot.empty()
            ? context.imported.cookedPath.parent_path() / "Thumbnails"
            : context.processedRoot / "Thumbnails";
        std::error_code ec;
        std::filesystem::create_directories(thumbDir, ec);
        const auto thumbPath = thumbDir / (context.imported.metadata.guid.ToString() + ".thumb");
        // Placeholder thumbnail marker (real GPU/CPU encode plugs in later).
        std::ofstream out(thumbPath, std::ios::binary);
        const char marker[] = "WETHUMB1";
        out.write(marker, sizeof(marker) - 1);
        out.write(reinterpret_cast<const char*>(context.imported.metadata.guid.bytes.data()), 16);
        result.asset.thumbnailPath = thumbPath;
#if WE_HAS_NLOHMANN_JSON
        result.asset.metadata.custom["thumbnail"] = thumbPath.string();
#endif
        if (progress) {
            progress(1.0f, "GenerateThumbnail");
        }
        return result;
    }
};

class SidecarWriterProcessor final : public IAssetProcessor {
public:
    [[nodiscard]] std::string_view GetProcessorId() const override { return "sidecar.write"; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return "Metadata Sidecar Writer"; }
    [[nodiscard]] ProcessStage GetStage() const override { return ProcessStage::WriteSidecars; }
    [[nodiscard]] bool CanProcess(we::runtime::assetimporter::AssetKind) const override { return true; }

    [[nodiscard]] ProcessResult Process(
        ProcessContext& context,
        const ProcessProgressCallback& progress) const override {
        ProcessResult result{};
        result.asset = context.imported;
        result.success = true;
        if (progress) {
            progress(0.5f, "WriteSidecars");
        }
        auto metaPath = result.asset.metadataPath;
        if (metaPath.empty()) {
            metaPath = result.asset.cookedPath.parent_path()
                / (result.asset.cookedPath.stem().string() + ".wemeta");
        }
        result.asset.metadata.modifiedUtc = result.asset.metadata.modifiedUtc.empty()
            ? result.asset.metadata.createdUtc
            : result.asset.metadata.modifiedUtc;
        if (!we::runtime::assetimporter::SaveMetadataJson(metaPath, result.asset.metadata)) {
            AddDiag(result, we::runtime::assetimporter::ImportSeverity::Warning,
                "sidecar.write_failed",
                "Failed to write .wemeta sidecar.",
                std::string(GetProcessorId()));
        } else {
            result.asset.metadataPath = metaPath;
        }
        return result;
    }
};

} // namespace

void RegisterBuiltinProcessors(ProcessorRegistry& registry) {
    registry.Register(std::make_shared<ValidateProcessor>());
    registry.Register(std::make_shared<DependencyAnalyzeProcessor>());
    registry.Register(std::make_shared<OptimizeProcessor>());
    registry.Register(std::make_shared<MipProcessor>());
    registry.Register(std::make_shared<LodProcessor>());
    registry.Register(std::make_shared<ThumbnailProcessor>());
    registry.Register(std::make_shared<SidecarWriterProcessor>());
}

} // namespace we::runtime::assetprocessors
