#include "AssetTools/AssetServiceHost.h"

namespace we::runtime::assettools {
namespace {

DiagnosticSeverity FromImportSeverity(we::runtime::assetimporter::ImportSeverity severity) {
    switch (severity) {
    case we::runtime::assetimporter::ImportSeverity::Warning:
        return DiagnosticSeverity::Warning;
    case we::runtime::assetimporter::ImportSeverity::Error:
        return DiagnosticSeverity::Error;
    case we::runtime::assetimporter::ImportSeverity::Fatal:
        return DiagnosticSeverity::Fatal;
    case we::runtime::assetimporter::ImportSeverity::Info:
    default:
        return DiagnosticSeverity::Info;
    }
}

} // namespace

AssetServiceHost::AssetServiceHost(AssetServiceHostConfig config)
    : m_Config(std::move(config)) {
    we::runtime::assetimporter::AssetImportServiceDependencies importDeps{};
    importDeps.engineVersion = m_Config.engineVersion;
    importDeps.workerThreads = m_Config.workerThreads;
    importDeps.onDiagnostic = [this](const we::runtime::assetimporter::ImportDiagnostic& d) {
        Emit(FromImportSeverity(d.severity), "Import", d.code, d.message);
    };
    m_Import = we::runtime::assetimporter::CreateAssetImportService(std::move(importDeps));

    we::runtime::assetprocessors::AssetProcessorServiceDependencies procDeps{};
    procDeps.engineVersion = m_Config.engineVersion;
    m_Processors = we::runtime::assetprocessors::CreateAssetProcessorService(std::move(procDeps));

    we::runtime::assetpipeline::AssetPipelineDependencies pipeDeps{};
    pipeDeps.importService = m_Import.get();
    pipeDeps.processorService = m_Processors.get();
    pipeDeps.platform = m_Config.platform;
    pipeDeps.engineVersion = m_Config.engineVersion;
    pipeDeps.cacheRoot = m_Config.cacheRoot;
    pipeDeps.workerThreads = m_Config.workerThreads;
    pipeDeps.onDiagnostic = [this](const we::runtime::assetpipeline::PipelineDiagnostic& d) {
        Emit(FromImportSeverity(d.severity), "Pipeline", d.code, d.message);
    };
    m_Pipeline = we::runtime::assetpipeline::CreateAssetPipeline(std::move(pipeDeps));

    we::runtime::assetcooker::AssetCookerDependencies cookDeps{};
    cookDeps.engineVersion = m_Config.engineVersion;
    cookDeps.onDiagnostic = [this](const we::runtime::assetcooker::CookDiagnostic& d) {
        Emit(FromImportSeverity(d.severity), "Cooker", d.code, d.message);
    };
    m_Cooker = we::runtime::assetcooker::CreateAssetCooker(std::move(cookDeps));
}

AssetServiceHost::~AssetServiceHost() {
    if (m_Pipeline) {
        m_Pipeline->Shutdown();
    }
    if (m_Import) {
        m_Import->Shutdown();
    }
}

we::runtime::assetimporter::IAssetImportService& AssetServiceHost::ImportService() {
    return *m_Import;
}

we::runtime::assetprocessors::IAssetProcessorService& AssetServiceHost::ProcessorService() {
    return *m_Processors;
}

we::runtime::assetpipeline::IAssetPipeline& AssetServiceHost::Pipeline() {
    return *m_Pipeline;
}

we::runtime::assetcooker::IAssetCooker& AssetServiceHost::Cooker() {
    return *m_Cooker;
}

void AssetServiceHost::Emit(
    DiagnosticSeverity severity,
    std::string source,
    std::string code,
    std::string message) const {
    if (!m_Config.onDiagnostic) {
        return;
    }
    AssetDiagnostic d{};
    d.severity = severity;
    d.source = std::move(source);
    d.code = std::move(code);
    d.message = std::move(message);
    m_Config.onDiagnostic(d);
}

std::unique_ptr<AssetServiceHost> CreateAssetServiceHost(AssetServiceHostConfig config) {
    return std::make_unique<AssetServiceHost>(std::move(config));
}

} // namespace we::runtime::assettools
