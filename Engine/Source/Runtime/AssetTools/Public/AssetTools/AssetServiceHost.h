#pragma once

#include "AssetCooker/IAssetCooker.h"
#include "AssetImporter/IAssetImportService.h"
#include "AssetPipeline/IAssetPipeline.h"
#include "AssetProcessors/IAssetProcessorService.h"
#include "AssetTools/Export.h"
#include "Platform/IPlatform.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace we::runtime::assettools {

/// Shared diagnostic severity used by CLI, Editor, and automation hosts.
enum class DiagnosticSeverity : uint32_t {
    Info = 0,
    Warning,
    Error,
    Fatal
};

struct ASSETTOOLS_API AssetDiagnostic {
    DiagnosticSeverity severity = DiagnosticSeverity::Info;
    std::string source; // "Import" | "Pipeline" | "Cooker" | "Tools"
    std::string code;
    std::string message;
};

using AssetDiagnosticCallback = std::function<void(const AssetDiagnostic&)>;

/// Dependency injection for the shared asset service graph.
struct ASSETTOOLS_API AssetServiceHostConfig {
    std::string engineVersion = "0.1.0";
    std::filesystem::path cacheRoot; // optional Intermediate/AssetPipeline override
    we::platform::IPlatform* platform = nullptr; // optional; required for watch
    AssetDiagnosticCallback onDiagnostic;
    unsigned workerThreads = 0;
};

/// Owns import → processors → pipeline → cooker services constructed once per host.
/// Editor, CLI (`we.exe`), automation, and CI all share this wiring.
class ASSETTOOLS_API AssetServiceHost {
public:
    explicit AssetServiceHost(AssetServiceHostConfig config = {});
    ~AssetServiceHost();

    AssetServiceHost(const AssetServiceHost&) = delete;
    AssetServiceHost& operator=(const AssetServiceHost&) = delete;

    [[nodiscard]] we::runtime::assetimporter::IAssetImportService& ImportService();
    [[nodiscard]] we::runtime::assetprocessors::IAssetProcessorService& ProcessorService();
    [[nodiscard]] we::runtime::assetpipeline::IAssetPipeline& Pipeline();
    [[nodiscard]] we::runtime::assetcooker::IAssetCooker& Cooker();

    [[nodiscard]] const std::string& EngineVersion() const noexcept { return m_Config.engineVersion; }

    void Emit(DiagnosticSeverity severity, std::string source, std::string code, std::string message) const;

private:
    AssetServiceHostConfig m_Config{};
    std::unique_ptr<we::runtime::assetimporter::IAssetImportService> m_Import;
    std::unique_ptr<we::runtime::assetprocessors::IAssetProcessorService> m_Processors;
    std::unique_ptr<we::runtime::assetpipeline::IAssetPipeline> m_Pipeline;
    std::unique_ptr<we::runtime::assetcooker::IAssetCooker> m_Cooker;
};

[[nodiscard]] ASSETTOOLS_API std::unique_ptr<AssetServiceHost> CreateAssetServiceHost(
    AssetServiceHostConfig config = {});

} // namespace we::runtime::assettools
