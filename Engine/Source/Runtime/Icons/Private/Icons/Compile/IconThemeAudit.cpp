#include "Icons/Compile/IconThemeAudit.h"
#include "Icons/Compile/IconCompileDetail.h"
#include "Icons/Core/IconTypes.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

namespace we::runtime::icons::compiling {
namespace {

using detail::IsFullColorIcon;
using detail::ParseLibGdxAtlasFile;
using detail::ParsedAtlasDescriptor;
using detail::ResolveRuntimeIconName;
using detail::RuntimeAliasesFor;

void AddFinding(IconAuditReport& report, IconAuditFinding::Severity severity, std::string category, std::string message)
{
    IconAuditFinding finding;
    finding.severity = severity;
    finding.category = std::move(category);
    finding.message = std::move(message);
    if (severity == IconAuditFinding::Severity::Error) {
        report.ok = false;
    }
    report.findings.push_back(std::move(finding));
}

const std::unordered_set<std::string>& CanonicalActionIcons()
{
    static const std::unordered_set<std::string> kIcons = {
        "save", "saveall", "openfolder", "newfile", "folder", "search", "settings",
        "play", "playsolid", "mediaplay", "pause", "stop",
        "copy", "paste", "undo", "redo",
        "move", "rotate", "scale",
        "close", "chevrondown", "chevronright",
        "contentbrowser", "outputlog", "terminal", "pivot", "addactor",
    };
    return kIcons;
}

} // namespace

IconAuditReport AuditIconAtlas(const std::filesystem::path& inputDir)
{
    IconAuditReport report;

    if (!std::filesystem::is_directory(inputDir)) {
        AddFinding(report, IconAuditFinding::Severity::Error, "input", "Atlas input directory does not exist");
        return report;
    }

    std::unordered_map<std::string, std::unordered_map<uint32_t, std::pair<uint16_t, uint16_t>>> iconSizesByTier;
    std::unordered_map<uint32_t, std::unordered_set<std::string>> tierRuntimeNames;
    std::unordered_set<std::string> allRuntimeNames;

    for (const uint32_t tierPx : kIconAtlasTiers) {
        const auto atlasPath = inputDir / ("ui_Atlas_" + std::to_string(tierPx) + ".atlas");
        if (!std::filesystem::exists(atlasPath)) {
            continue;
        }

        ParsedAtlasDescriptor descriptor;
        std::string parseError;
        if (!ParseLibGdxAtlasFile(atlasPath, inputDir, descriptor, parseError)) {
            AddFinding(report, IconAuditFinding::Severity::Error, "parse", parseError);
            continue;
        }

        ++report.tierCount;

        for (const auto& region : descriptor.regions) {
            const std::string runtimeName = ResolveRuntimeIconName(region.sourceName);
            tierRuntimeNames[tierPx].insert(runtimeName);
            allRuntimeNames.insert(runtimeName);

            iconSizesByTier[runtimeName][tierPx] = {region.width, region.height};

            if (IsFullColorIcon(runtimeName)
                && runtimeName != "windeffects"
                && runtimeName != "windlogo"
                && runtimeName.rfind("3d", 0) != 0) {
                AddFinding(
                    report,
                    IconAuditFinding::Severity::Warning,
                    "full-color",
                    "Icon '" + runtimeName + "' is flagged full-color; verify it should bypass mono tinting");
            }

            if (region.width != region.height) {
                AddFinding(
                    report,
                    IconAuditFinding::Severity::Warning,
                    "aspect",
                    "Non-square region '" + region.sourceName + "' at tier " + std::to_string(tierPx));
            }

            if (region.width < tierPx || region.height < tierPx) {
                AddFinding(
                    report,
                    IconAuditFinding::Severity::Warning,
                    "size",
                    "Region '" + runtimeName + "' at tier " + std::to_string(tierPx)
                        + " is smaller than tier (" + std::to_string(region.width) + "x"
                        + std::to_string(region.height) + ")");
            }
        }
    }

    report.iconCount = static_cast<uint32_t>(allRuntimeNames.size());

    if (report.tierCount == 0) {
        AddFinding(report, IconAuditFinding::Severity::Error, "input", "No atlas tiers found");
        return report;
    }

    const auto& tier16 = tierRuntimeNames[16];
    for (const auto& canonical : CanonicalActionIcons()) {
        if (tier16.find(canonical) == tier16.end()) {
            AddFinding(
                report,
                IconAuditFinding::Severity::Warning,
                "missing",
                "Canonical icon '" + canonical + "' missing from tier 16 atlas");
        }
    }

    for (const auto& [runtimeName, sizesByTier] : iconSizesByTier) {
        if (sizesByTier.size() < 2) {
            continue;
        }

        std::pair<uint16_t, uint16_t> reference{};
        bool hasReference = false;
        for (const auto& [tier, size] : sizesByTier) {
            if (!hasReference) {
                reference = size;
                hasReference = true;
                continue;
            }
            const float refAspect = static_cast<float>(reference.first) / static_cast<float>(reference.second);
            const float tierAspect = static_cast<float>(size.first) / static_cast<float>(size.second);
            if (std::abs(refAspect - tierAspect) > 0.05f) {
                AddFinding(
                    report,
                    IconAuditFinding::Severity::Warning,
                    "proportion",
                    "Icon '" + runtimeName + "' aspect ratio differs across tiers");
                break;
            }
        }
    }

    std::unordered_map<std::string, std::string> aliasToCanonical;
    for (const auto& runtimeName : allRuntimeNames) {
        for (const auto& alias : RuntimeAliasesFor(runtimeName)) {
            if (alias == runtimeName) {
                continue;
            }
            const auto [it, inserted] = aliasToCanonical.emplace(alias, runtimeName);
            if (!inserted && it->second != runtimeName) {
                AddFinding(
                    report,
                    IconAuditFinding::Severity::Warning,
                    "alias",
                    "Alias '" + alias + "' maps to both '" + it->second + "' and '" + runtimeName + "'");
            }
        }
    }

    return report;
}

void PrintIconAuditReport(const IconAuditReport& report)
{
    for (const auto& finding : report.findings) {
        const char* level = finding.severity == IconAuditFinding::Severity::Error ? "ERROR" : "WARN";
        std::cout << '[' << level << "] " << finding.category << ": " << finding.message << '\n';
    }
    std::cout << "Icon audit: " << report.iconCount << " icons across " << report.tierCount
              << " tiers, " << report.findings.size() << " findings"
              << (report.ok ? " (ok)" : " (failed)") << '\n';
}

} // namespace we::runtime::icons::compiling
