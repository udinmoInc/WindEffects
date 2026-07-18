#pragma once
#include <string>
#include <nlohmann/json.h>
#include <filesystem>
#include <fstream>
#include "Core/Logger.h"
#include "Core/Paths.h"

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

    void Load(const std::string& configPath = {}) {
        std::filesystem::path resolved = configPath.empty()
            ? we::core::PathService::Get().EngineConfigRoot() / "CrashReporter" / "config.json"
            : we::core::PathService::FromUtf8(configPath);
        const std::string pathStr = we::core::PathService::ToUtf8(resolved);
        if (!std::filesystem::exists(resolved)) {
            HE_WARN("CrashReporter config not found at " + pathStr);
            return;
        }

        try {
            std::ifstream f(resolved, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

            auto decodeUtf16Le = [](const std::string& raw, size_t startOffset) {
                std::string utf8;
                utf8.reserve((raw.size() - startOffset) / 2);
                for (size_t i = startOffset; i + 1 < raw.size(); i += 2) {
                    const wchar_t ch = static_cast<wchar_t>(
                        static_cast<unsigned char>(raw[i])
                        | (static_cast<unsigned char>(raw[i + 1]) << 8));
                    if (ch <= 0x7F) {
                        utf8.push_back(static_cast<char>(ch));
                    } else if (ch <= 0x7FF) {
                        utf8.push_back(static_cast<char>(0xC0 | ((ch >> 6) & 0x1F)));
                        utf8.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
                    } else {
                        utf8.push_back(static_cast<char>(0xE0 | ((ch >> 12) & 0x0F)));
                        utf8.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
                        utf8.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
                    }
                }
                return utf8;
            };

            if (content.size() >= 2
                && static_cast<unsigned char>(content[0]) == 0xFF
                && static_cast<unsigned char>(content[1]) == 0xFE) {
                content = decodeUtf16Le(content, 2);
            } else if (content.size() >= 4
                && content[0] == '{'
                && content[1] == '\0'
                && content[2] == '\n'
                && content[3] == '\0') {
                content = decodeUtf16Le(content, 0);
            }

            nlohmann::json j = nlohmann::json::parse(content);

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
                
                HE_INFO("Loaded CrashReporter config from " + pathStr);
            } catch (const std::exception& e) {
                HE_ERROR("Failed to parse CrashReporter config: " + std::string(e.what()));
            }
    }

    const CrashReporterConfig& GetConfig() const {
        return m_Config;
    }

private:
    CrashReporterConfig m_Config;
};

} // namespace we::programs::crashreporter
