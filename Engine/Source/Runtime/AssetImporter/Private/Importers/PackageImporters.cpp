#include "Importers/BuiltinImporters.h"

#include "ImportHelpers.h"
#include "AssetImporter/AssetPackage.h"
#include "AssetImporter/ImporterRegistry.h"

#include <algorithm>

namespace we::runtime::assetimporter {
namespace {

class PackageSourceImporter final : public IAssetImporter {
public:
    PackageSourceImporter(
        std::string id,
        std::string displayName,
        AssetKind kind,
        std::vector<std::string> extensions,
        std::string payloadContentType,
        int priority = 50)
        : m_Id(std::move(id))
        , m_DisplayName(std::move(displayName))
        , m_Kind(kind)
        , m_Extensions(std::move(extensions))
        , m_PayloadContentType(std::move(payloadContentType))
        , m_Priority(priority) {
    }

    [[nodiscard]] std::string_view GetImporterId() const override { return m_Id; }
    [[nodiscard]] std::string_view GetDisplayName() const override { return m_DisplayName; }
    [[nodiscard]] AssetKind GetAssetKind() const override { return m_Kind; }
    [[nodiscard]] std::vector<std::string> GetSupportedExtensions() const override { return m_Extensions; }
    [[nodiscard]] int GetPriority() const override { return m_Priority; }

    [[nodiscard]] bool CanImport(const std::filesystem::path& sourcePath) const override {
        std::string ext = sourcePath.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        for (const auto& supported : m_Extensions) {
            if (ext == supported) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] ImportResult Import(
        const ImportRequest& request,
        const ImportProgressCallback& progress) const override {
        ImportResult result{};
        result.jobId = request.existingGuid.IsNil() ? AssetGuid::Generate() : request.existingGuid;

        if (!std::filesystem::exists(request.sourcePath)) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "source.missing",
                "Source file does not exist.", request.sourcePath.string());
            return result;
        }
        if (request.outputDirectory.empty()) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "output.missing",
                "Output directory is required.");
            return result;
        }

        detail::ReportProgress(progress, result.jobId, 0.1f, "ReadingSource");
        auto bytes = detail::ReadAllBytes(request.sourcePath);
        if (bytes.empty() && std::filesystem::file_size(request.sourcePath) > 0) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "source.read_failed",
                "Failed to read source file.", request.sourcePath.string());
            return result;
        }

        detail::ReportProgress(progress, result.jobId, 0.5f, "BuildingPackage");
        AssetPackage package{};
        package.metadata = detail::BuildBaseMetadata(request, m_Kind, m_Id, "0.1.0");
        package.metadata.guid = result.jobId;
        package.payload = std::move(bytes);
        package.payloadContentType = m_PayloadContentType;

        const auto cookedPath = detail::MakeOutputPath(request, m_Kind, ".weasset");
        if (!request.overwriteExisting && std::filesystem::exists(cookedPath)) {
            detail::AddDiagnostic(result, ImportSeverity::Error, "output.exists",
                "Output already exists and overwrite is disabled.", cookedPath.string());
            return result;
        }

        detail::ReportProgress(progress, result.jobId, 0.8f, "WritingPackage");
        if (!WriteAssetPackage(cookedPath, package)) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "output.write_failed",
                "Failed to write .weasset package.", cookedPath.string());
            return result;
        }

        const auto metaPath = cookedPath;
        result.success = true;
        result.asset.metadata = package.metadata;
        result.asset.cookedPath = cookedPath;
        result.asset.metadataPath = metaPath;
        result.asset.cookedBytes = static_cast<uint64_t>(package.payload.size());
        detail::ReportProgress(progress, result.jobId, 1.0f, "Completed");
        return result;
    }

private:
    std::string m_Id;
    std::string m_DisplayName;
    AssetKind m_Kind;
    std::vector<std::string> m_Extensions;
    std::string m_PayloadContentType;
    int m_Priority = 50;
};

} // namespace

std::shared_ptr<IAssetImporter> CreateTextureImporter() {
    return std::make_shared<PackageSourceImporter>(
        "texture.raw",
        "Texture Importer",
        AssetKind::Texture,
        std::vector<std::string>{ ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr", ".exr", ".webp" },
        "application/vnd.windeffects.texture.source.v1",
        80);
}

std::shared_ptr<IAssetImporter> CreateMeshImporter() {
    return std::make_shared<PackageSourceImporter>(
        "mesh.raw",
        "Mesh Importer",
        AssetKind::StaticMesh,
        std::vector<std::string>{ ".obj", ".fbx", ".gltf", ".glb", ".dae" },
        "application/vnd.windeffects.mesh.source.v1",
        70);
}

std::shared_ptr<IAssetImporter> CreateAnimationImporter() {
    return std::make_shared<PackageSourceImporter>(
        "animation.raw",
        "Animation Importer",
        AssetKind::Animation,
        std::vector<std::string>{ ".anim", ".animation" },
        "application/vnd.windeffects.animation.source.v1",
        60);
}

std::shared_ptr<IAssetImporter> CreateAudioImporter() {
    return std::make_shared<PackageSourceImporter>(
        "audio.raw",
        "Audio Importer",
        AssetKind::Audio,
        std::vector<std::string>{ ".wav", ".mp3", ".ogg", ".flac" },
        "application/vnd.windeffects.audio.source.v1",
        70);
}

std::shared_ptr<IAssetImporter> CreateShaderImporter() {
    return std::make_shared<PackageSourceImporter>(
        "shader.raw",
        "Shader Importer",
        AssetKind::Shader,
        std::vector<std::string>{ ".hlsl", ".glsl", ".spv", ".shader" },
        "application/vnd.windeffects.shader.source.v1",
        70);
}

std::shared_ptr<IAssetImporter> CreateMaterialImporter() {
    return std::make_shared<PackageSourceImporter>(
        "material.raw",
        "Material Importer",
        AssetKind::Material,
        std::vector<std::string>{ ".mat", ".matinst", ".mi" },
        "application/vnd.windeffects.material.source.v1",
        70);
}

std::shared_ptr<IAssetImporter> CreateSceneImporter() {
    return std::make_shared<PackageSourceImporter>(
        "scene.raw",
        "Scene Importer",
        AssetKind::Scene,
        std::vector<std::string>{ ".scene", ".level", ".umap" },
        "application/vnd.windeffects.scene.source.v1",
        70);
}

std::shared_ptr<IAssetImporter> CreateRawBinaryImporter() {
    return std::make_shared<PackageSourceImporter>(
        "raw.binary",
        "Raw Binary Importer",
        AssetKind::RawBinary,
        std::vector<std::string>{ ".bin", ".dat", ".raw" },
        "application/octet-stream",
        10);
}

} // namespace we::runtime::assetimporter
