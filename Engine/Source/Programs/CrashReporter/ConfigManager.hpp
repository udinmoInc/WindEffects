#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include "Core/Logger.hpp"

namespace we::programs::crashreporter {

struct CrashReporterConfig {
    std::string companyName = "WindEffects";
    std::string engineName = "WindEffects Engine";
    std::string supportEmail = "support@windeffects.com";
    std::string supportWebsite = "https://windeffects.com";
    std::string issueTracker = "https://windeffects.com/issues";
    std::string documentation = "https://docs.windeffects.com";
    std::string discord = "https://discord.gg/...";
    std::string github = "https://github.com/...";
    
    bool allowRestart = true;
    bool allowZipExport = true;
    bool allowSendReport = true;
    bool collectScreenshot = true;
    bool collectLogs = true;
    bool collectDump = true;
    bool collectSystemInfo = true;
    
    int maxRecentLogs = 200;
    std::string theme = "Dark";
};

class ConfigManager {
public:
    static ConfigManager& Get() {
        static ConfigManager instance;
        return instance;
    }

    void Load(const std::string& configPath = "Engine/Config/CrashReporter/config.json") {
        if (std::filesystem::exists(configPath)) {
            try {
                std::ifstream f(configPath);
                nlohmann::json j = nlohmann::json::parse(f);

                auto assign_str = [&](const std::string& key, std::string& out) {
                    if (j.contains(key) && j[key].is_string()) {
                        out = j[key].get<std::string>();
                    }
                };
                
                auto assign_bool = [&](const std::string& key, bool& out) {
                    if (j.contains(key) && j[key].is_boolean()) {
                        out = j[key].get<bool>();
                    }
                };
                
                auto assign_int = [&](const std::string& key, int& out) {
                    if (j.contains(key) && j[key].is_number_integer()) {
                        out = j[key].get<int>();
                    }
                };

                assign_str("companyName", m_Config.companyName);
                assign_str("engineName", m_Config.engineName);
                assign_str("supportEmail", m_Config.supportEmail);
                assign_str("supportWebsite", m_Config.supportWebsite);
                assign_str("issueTracker", m_Config.issueTracker);
                assign_str("documentation", m_Config.documentation);
                assign_str("discord", m_Config.discord);
                assign_str("github", m_Config.github);
                assign_str("theme", m_Config.theme);

                assign_bool("allowRestart", m_Config.allowRestart);
                assign_bool("allowZipExport", m_Config.allowZipExport);
                assign_bool("allowSendReport", m_Config.allowSendReport);
                assign_bool("collectScreenshot", m_Config.collectScreenshot);
                assign_bool("collectLogs", m_Config.collectLogs);
                assign_bool("collectDump", m_Config.collectDump);
                assign_bool("collectSystemInfo", m_Config.collectSystemInfo);

                assign_int("maxRecentLogs", m_Config.maxRecentLogs);
                
                HE_INFO("Loaded CrashReporter config from " + configPath);
            } catch (const std::exception& e) {
                HE_ERROR("Failed to parse CrashReporter config: " + std::string(e.what()));
            }
        } else {
            HE_WARN("CrashReporter config not found at " + configPath);
        }
    }

    const CrashReporterConfig& GetConfig() const {
        return m_Config;
    }

private:
    CrashReporterConfig m_Config;
};

} // namespace we::programs::crashreporter
