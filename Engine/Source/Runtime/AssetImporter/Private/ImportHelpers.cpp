#include "ImportHelpers.h"

#include "AssetImporter/AssetPackage.h"
#include "AssetImporter/Types.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace we::runtime::assetimporter::detail {

std::string NowUtc() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const std::time_t t = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::filesystem::path MakeOutputPath(
    const ImportRequest& request,
    AssetKind kind,
    std::string_view preferredExtension) {
    std::string name = request.assetName;
    if (name.empty()) {
        name = request.sourcePath.stem().string();
    }
    const std::string_view ext = preferredExtension.empty()
        ? AssetKindNativeExtension(kind)
        : preferredExtension;
    return request.outputDirectory / (name + std::string(ext));
}

void ReportProgress(
    const ImportProgressCallback& cb,
    const AssetGuid& jobId,
    float fraction,
    std::string stage,
    std::string detail) {
    if (!cb) {
        return;
    }
    ImportProgress progress{};
    progress.jobId = jobId;
    progress.state = ImportJobState::Running;
    progress.fraction = fraction < 0.0f ? 0.0f : (fraction > 1.0f ? 1.0f : fraction);
    progress.stage = std::move(stage);
    progress.detail = std::move(detail);
    cb(progress);
}

void AddDiagnostic(
    ImportResult& result,
    ImportSeverity severity,
    std::string code,
    std::string message,
    std::string contextPath) {
    ImportDiagnostic d{};
    d.severity = severity;
    d.code = std::move(code);
    d.message = std::move(message);
    d.contextPath = std::move(contextPath);
    result.diagnostics.push_back(std::move(d));
}

AssetMetadata BuildBaseMetadata(
    const ImportRequest& request,
    AssetKind kind,
    std::string_view importerId,
    std::string_view engineVersion) {
    AssetMetadata metadata{};
    metadata.guid = request.existingGuid.IsNil() ? AssetGuid::Generate() : request.existingGuid;
    metadata.kind = kind;
    metadata.displayName = request.assetName.empty()
        ? request.sourcePath.stem().string()
        : request.assetName;
    metadata.sourcePath = request.sourcePath.string();
    metadata.sourceHash = ComputeFileHashHex(request.sourcePath);
    metadata.importerId = std::string(importerId);
    metadata.importerVersion = "1.0.0";
    metadata.engineVersion = std::string(engineVersion);
    metadata.createdUtc = NowUtc();
    metadata.modifiedUtc = metadata.createdUtc;
#if WE_HAS_NLOHMANN_JSON
    metadata.importSettings = request.settings;
#endif
    return metadata;
}

bool WriteBytes(const std::filesystem::path& path, const void* data, size_t size) {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        return false;
    }
    if (size > 0 && data != nullptr) {
        out.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    }
    return static_cast<bool>(out);
}

std::vector<std::byte> ReadAllBytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) {
        return {};
    }
    const auto size = static_cast<size_t>(in.tellg());
    in.seekg(0);
    std::vector<std::byte> data(size);
    if (size > 0) {
        in.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    }
    return data;
}

} // namespace we::runtime::assetimporter::detail
