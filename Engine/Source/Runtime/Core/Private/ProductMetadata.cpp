#include "Core/ProductMetadata.h"
#include "Core/Logger.h"
#include "Core/Paths.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>

#if WE_HAS_NLOHMANN_JSON
#include <nlohmann/json.h>
#endif

namespace we::core {

std::string ProductMetadata::GetFullVersionString() const {
    return product.version + "." + product.build;
}

std::string ProductMetadata::GetFullProductString() const {
    return product.displayName + " " + GetFullVersionString();
}

std::string ProductMetadata::GetExecutableDisplayName() const {
    if (!executableIdentity.fileDescription.empty()) {
        return executableIdentity.fileDescription;
    }
    return product.displayName;
}

std::string ProductMetadata::GetCopyrightString() const {
    return company.copyright;
}

uint32_t ProductMetadata::GetVersionAsUint32() const {
    uint32_t version = 0;
    std::istringstream iss(product.version);
    std::string token;

    for (int i = 0; i < 3 && std::getline(iss, token, '.'); ++i) {
        try {
            uint32_t part = static_cast<uint32_t>(std::stoul(token));
            version |= (part << (8 * (2 - i)));
        } catch (...) {
            break;
        }
    }
    return version;
}

uint32_t ProductMetadata::GetBuildAsUint32() const {
    try {
        return static_cast<uint32_t>(std::stoul(product.build));
    } catch (...) {
        return 0;
    }
}

ProductMetadataService& ProductMetadataService::Get() {
    static ProductMetadataService instance;
    return instance;
}

bool ProductMetadataService::Initialize(const std::string& configPath) {
    if (m_Initialized) {
        return true;
    }

    std::string path = configPath;
    if (path.empty()) {
        auto& paths = PathService::Get();
        path = PathService::ToGeneric(paths.EngineConfigRoot() / "ProductMetadata.json");
    }

    if (!LoadFromJson(path)) {
        HE_WARN("[ProductMetadata] Failed to load from " + path + ", using defaults");
        SetDefaults();
    }

    PopulateBuildInfo();
    m_Initialized = true;

    HE_INFO("[ProductMetadata] " + m_Metadata.GetFullProductString());
    HE_INFO("[ProductMetadata] " + m_Metadata.GetCopyrightString());
    HE_INFO("[ProductMetadata] Engine GUID: " + m_Metadata.engine.guid);

    return true;
}

void ProductMetadataService::Shutdown() {
    m_Initialized = false;
    m_Metadata = ProductMetadata{};
}

void ProductMetadataService::SetExecutableRole(ExecutableRole role) {
    m_Metadata.executableRole = role;
}

void ProductMetadataService::SetExecutableIdentity(const ExecutableIdentity& identity) {
    m_Metadata.executableIdentity = identity;
}

void ProductMetadataService::SetCrashedApplicationMetadata(const ProductMetadata& crashedAppMetadata) {
    m_CrashedApplicationMetadata = crashedAppMetadata;
    m_HasCrashedApplicationMetadata = true;
}

const ProductMetadata* ProductMetadataService::GetCrashedApplicationMetadata() const {
    return m_HasCrashedApplicationMetadata ? &m_CrashedApplicationMetadata : nullptr;
}

void ProductMetadataService::ClearCrashedApplicationMetadata() {
    m_CrashedApplicationMetadata = ProductMetadata{};
    m_HasCrashedApplicationMetadata = false;
}

bool ProductMetadataService::LoadFromSerialized(const std::string& serialized) {
#if !WE_HAS_NLOHMANN_JSON
    (void)serialized;
    return false;
#else
    try {
        nlohmann::json root = nlohmann::json::parse(serialized);

        if (root.contains("product")) {
            auto product = root["product"];
            m_Metadata.product.name = product.value("name", "WindEffects");
            m_Metadata.product.codename = product.value("codename", "WeEngine");
            m_Metadata.product.displayName = product.value("displayName", "WindEffects Engine");
            m_Metadata.product.version = product.value("version", "1.0.0");
            m_Metadata.product.build = product.value("build", "1001");
            m_Metadata.product.identifier = product.value("identifier", "com.udinmo.windeffects");
        }

        if (root.contains("company")) {
            auto company = root["company"];
            m_Metadata.company.name = company.value("name", "Udinmo, Inc.");
            m_Metadata.company.website = company.value("website", "https://udinmo.com");
            m_Metadata.company.copyright = company.value("copyright", "© 2026 Udinmo, Inc. All rights reserved.");
        }

        if (root.contains("engine")) {
            auto engine = root["engine"];
            m_Metadata.engine.guid = engine.value("guid", "A7B3C8D2-1E4F-4A5B-8C9D-0E1F2A3B4C5D");
            m_Metadata.engine.buildId = engine.value("buildId", "");
            m_Metadata.engine.provenanceSignature = engine.value("provenanceSignature", "WE-PD-2026");
        }

        if (root.contains("build")) {
            auto build = root["build"];
            m_Metadata.build.configuration = build.value("configuration", "Debug");
            m_Metadata.build.timestamp = build.value("timestamp", "");
            m_Metadata.build.gitCommit = build.value("gitCommit", "");
            m_Metadata.build.gitBranch = build.value("gitBranch", "");
            m_Metadata.build.buildRevision = build.value("buildRevision", "");
        }

        if (root.contains("executableRole")) {
            m_Metadata.executableRole = static_cast<ExecutableRole>(root["executableRole"].get<uint8_t>());
        }

        if (root.contains("executableIdentity")) {
            auto identity = root["executableIdentity"];
            m_Metadata.executableIdentity.fileDescription = identity.value("fileDescription", "");
            m_Metadata.executableIdentity.internalName = identity.value("internalName", "");
            m_Metadata.executableIdentity.originalFilename = identity.value("originalFilename", "");
            m_Metadata.executableIdentity.windowTitleBase = identity.value("windowTitleBase", "");
        }

        m_Initialized = true;
        return true;
    } catch (const std::exception& e) {
        HE_ERROR("[ProductMetadata] Failed to load from serialized: " + std::string(e.what()));
        return false;
    }
#endif
}

std::string ProductMetadataService::Serialize() const {
#if !WE_HAS_NLOHMANN_JSON
    return "{}";
#else
    try {
        nlohmann::json root;
        
        root["product"]["name"] = m_Metadata.product.name;
        root["product"]["codename"] = m_Metadata.product.codename;
        root["product"]["displayName"] = m_Metadata.product.displayName;
        root["product"]["version"] = m_Metadata.product.version;
        root["product"]["build"] = m_Metadata.product.build;
        root["product"]["identifier"] = m_Metadata.product.identifier;

        root["company"]["name"] = m_Metadata.company.name;
        root["company"]["website"] = m_Metadata.company.website;
        root["company"]["copyright"] = m_Metadata.company.copyright;

        root["engine"]["guid"] = m_Metadata.engine.guid;
        root["engine"]["buildId"] = m_Metadata.engine.buildId;
        root["engine"]["provenanceSignature"] = m_Metadata.engine.provenanceSignature;

        root["build"]["configuration"] = m_Metadata.build.configuration;
        root["build"]["timestamp"] = m_Metadata.build.timestamp;
        root["build"]["gitCommit"] = m_Metadata.build.gitCommit;
        root["build"]["gitBranch"] = m_Metadata.build.gitBranch;
        root["build"]["buildRevision"] = m_Metadata.build.buildRevision;

        root["executableRole"] = static_cast<uint8_t>(m_Metadata.executableRole);

        root["executableIdentity"]["fileDescription"] = m_Metadata.executableIdentity.fileDescription;
        root["executableIdentity"]["internalName"] = m_Metadata.executableIdentity.internalName;
        root["executableIdentity"]["originalFilename"] = m_Metadata.executableIdentity.originalFilename;
        root["executableIdentity"]["windowTitleBase"] = m_Metadata.executableIdentity.windowTitleBase;

        return root.dump(2);
    } catch (const std::exception& e) {
        HE_ERROR("[ProductMetadata] Failed to serialize: " + std::string(e.what()));
        return "{}";
    }
#endif
}

bool ProductMetadataService::LoadFromJson(const std::string& path) {
#if !WE_HAS_NLOHMANN_JSON
    (void)path;
    return false;
#else
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json root;
        file >> root;

        if (root.contains("product")) {
            auto product = root["product"];
            m_Metadata.product.name = product.value("name", "WindEffects");
            m_Metadata.product.codename = product.value("codename", "WeEngine");
            m_Metadata.product.displayName = product.value("displayName", "WindEffects Engine");
            m_Metadata.product.version = product.value("version", "1.0.0");
            m_Metadata.product.build = product.value("build", "1001");
            m_Metadata.product.identifier = product.value("identifier", "com.udinmo.windeffects");
        }

        if (root.contains("build")) {
            auto build = root["build"];
            m_Metadata.build.configuration = build.value("configuration", "Debug");
            m_Metadata.build.timestamp = build.value("timestamp", "");
            m_Metadata.build.gitCommit = build.value("gitCommit", "");
            m_Metadata.build.gitBranch = build.value("gitBranch", "");
            m_Metadata.build.buildRevision = build.value("buildRevision", "");
        }

        if (root.contains("company")) {
            auto company = root["company"];
            m_Metadata.company.name = company.value("name", "Udinmo, Inc.");
            m_Metadata.company.website = company.value("website", "https://udinmo.com");
            m_Metadata.company.copyright = company.value("copyright", "© 2026 Udinmo, Inc. All rights reserved.");
        }

        if (root.contains("engine")) {
            auto engine = root["engine"];
            m_Metadata.engine.guid = engine.value("guid", "A7B3C8D2-1E4F-4A5B-8C9D-0E1F2A3B4C5D");
            m_Metadata.engine.buildId = engine.value("buildId", "");
            m_Metadata.engine.provenanceSignature = engine.value("provenanceSignature", "WE-PD-2026");
        }

        return true;
    } catch (const std::exception& e) {
        HE_ERROR("[ProductMetadata] JSON parse error: " + std::string(e.what()));
        return false;
    }
#endif
}

void ProductMetadataService::SetDefaults() {
    m_Metadata.product.name = "WindEffects";
    m_Metadata.product.codename = "WeEngine";
    m_Metadata.product.displayName = "WindEffects Engine";
    m_Metadata.product.version = "1.0.0";
    m_Metadata.product.build = "1001";
    m_Metadata.product.identifier = "com.udinmo.windeffects";

    m_Metadata.company.name = "Udinmo, Inc.";
    m_Metadata.company.website = "https://udinmo.com";
    m_Metadata.company.copyright = "© 2026 Udinmo, Inc. All rights reserved.";

    m_Metadata.engine.guid = "A7B3C8D2-1E4F-4A5B-8C9D-0E1F2A3B4C5D";
    m_Metadata.engine.buildId = "";
    m_Metadata.engine.provenanceSignature = "WE-PD-2026";

    m_Metadata.build.configuration = "Debug";
    m_Metadata.build.buildRevision = "";
}

void ProductMetadataService::PopulateBuildInfo() {
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
    m_Metadata.build.timestamp = oss.str();

#ifdef _WIN32
#ifdef _DEBUG
    m_Metadata.build.configuration = "Debug";
#else
    m_Metadata.build.configuration = "Release";
#endif
#endif
}

bool ProductMetadataService::SaveCrashContextMetadata(const std::string& outputPath) const {
#if !WE_HAS_NLOHMANN_JSON
    (void)outputPath;
    return false;
#else
    try {
        std::string serialized = Serialize();
        std::ofstream file(outputPath);
        if (!file.is_open()) {
            HE_ERROR("[ProductMetadata] Failed to open crash context file: " + outputPath);
            return false;
        }
        file << serialized;
        file.close();
        HE_INFO("[ProductMetadata] Saved crash context metadata to: " + outputPath);
        return true;
    } catch (const std::exception& e) {
        HE_ERROR("[ProductMetadata] Failed to save crash context: " + std::string(e.what()));
        return false;
    }
#endif
}

} // namespace we::core
