#pragma once

#include "Core/Export.h"

#include <string>
#include <cstdint>

namespace we::core {

enum class ExecutableRole : uint8_t {
    Unknown = 0,
    Engine,
    Editor,
    Launcher,
    CrashReporter,
    Tool,
    AssetCompiler,
    ShaderCompiler,
    Cooker,
    Packager,
    ReflectionHardening,
    SerializationHardening,
    Custom = 255,
};

struct ExecutableIdentity {
    std::string fileDescription;
    std::string internalName;
    std::string originalFilename;
    std::string windowTitleBase;
};

struct CORE_API ProductMetadata {
    struct ProductInfo {
        std::string name;
        std::string codename;
        std::string displayName;
        std::string version;
        std::string build;
        std::string identifier;
    };

    struct CompanyInfo {
        std::string name;
        std::string website;
        std::string copyright;
    };

    struct EngineInfo {
        std::string guid;
        std::string buildId;
        std::string provenanceSignature;
    };

    struct BuildInfo {
        std::string configuration;
        std::string timestamp;
        std::string gitCommit;
        std::string gitBranch;
        std::string buildRevision;
    };

    ProductInfo product;
    CompanyInfo company;
    EngineInfo engine;
    BuildInfo build;

    ExecutableRole executableRole = ExecutableRole::Unknown;
    ExecutableIdentity executableIdentity;

    [[nodiscard]] std::string GetFullVersionString() const;
    [[nodiscard]] std::string GetFullProductString() const;
    [[nodiscard]] std::string GetCopyrightString() const;
    [[nodiscard]] uint32_t GetVersionAsUint32() const;
    [[nodiscard]] uint32_t GetBuildAsUint32() const;
    [[nodiscard]] std::string GetExecutableDisplayName() const;
};

class CORE_API ProductMetadataService {
public:
    static ProductMetadataService& Get();

    bool Initialize(const std::string& configPath = "");
    void Shutdown();

    void SetExecutableRole(ExecutableRole role);
    void SetExecutableIdentity(const ExecutableIdentity& identity);

    bool LoadFromSerialized(const std::string& serialized);
    [[nodiscard]] std::string Serialize() const;

    void SetCrashedApplicationMetadata(const ProductMetadata& crashedAppMetadata);
    [[nodiscard]] const ProductMetadata* GetCrashedApplicationMetadata() const;
    void ClearCrashedApplicationMetadata();

    bool SaveCrashContextMetadata(const std::string& outputPath) const;

    [[nodiscard]] bool IsInitialized() const { return m_Initialized; }
    [[nodiscard]] const ProductMetadata& GetMetadata() const { return m_Metadata; }

    [[nodiscard]] const ProductMetadata::ProductInfo& GetProduct() const { return m_Metadata.product; }
    [[nodiscard]] const ProductMetadata::CompanyInfo& GetCompany() const { return m_Metadata.company; }
    [[nodiscard]] const ProductMetadata::EngineInfo& GetEngine() const { return m_Metadata.engine; }
    [[nodiscard]] const ProductMetadata::BuildInfo& GetBuild() const { return m_Metadata.build; }
    [[nodiscard]] ExecutableRole GetExecutableRole() const { return m_Metadata.executableRole; }
    [[nodiscard]] const ExecutableIdentity& GetExecutableIdentity() const { return m_Metadata.executableIdentity; }

private:
    ProductMetadataService() = default;
    ~ProductMetadataService() = default;

    bool LoadFromJson(const std::string& path);
    void PopulateBuildInfo();
    void SetDefaults();

    ProductMetadata m_Metadata;
    ProductMetadata m_CrashedApplicationMetadata;
    bool m_HasCrashedApplicationMetadata = false;
    bool m_Initialized = false;
};

} // namespace we::core
