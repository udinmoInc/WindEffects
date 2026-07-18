#include "UI/Shell/LauncherShell.h"

#include "UI/Shell/LauncherHelpers.h"
#include "UI/Pages/LauncherPages.h"
#include "UI/Pages/Projects/ProjectsPage.h"

#include "KindUI/App/ViewHost.h"
#include "KindUI/Declarative/UI.h"
#include "UI/Pages/CreateProject/CreateProjectViews.h"
#include "UI/Pages/Library/ManagerViews.h"
#include "UI/Pages/Settings/SettingsViews.h"
#include "Util/LauncherMaintenance.h"
#include "Util/PathUtils.h"

#include "KindUI/Core/EventSystem.h"
#include "KindUI/Core/Icon.h"
#include "KindUI/Core/PaintContext.h"
#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/ScrollLayout.h"
#include "KindUI/Layout/Spacer.h"
#include "Platform/PlatformSDK.h"
#include "KindUI/Widgets/Label.h"
#include "KindUI/Widgets/TextBox.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Theming/StyleRole.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string_view>

using namespace we::runtime::kindui;

namespace we::programs::welauncher {

using we::runtime::kindui::ColorToken;
void LauncherShell::CommitCreateProject() {
    m_Context->Settings().Settings().defaultTemplateId = m_WizardTemplateId;
    m_Context->Settings().Settings().qualityPreset = m_WizardQuality;
    PersistLauncherSettings({});
    const auto result = m_Context->Projects().CreateProject(
        m_WizardName,
        m_WizardTemplateId,
        PathUtils::FromUtf8(m_WizardLocation));
    SetStatus(result.message);
    if (result.success) {
        CloseModal();
        RefreshProjectList();
    }
}

void LauncherShell::SelectWizardTemplateByDelta(int delta) {
    const auto& templates = m_Context->Templates().Templates();
    const std::string query = ToLowerCopy(m_WizardTemplateQuery);
    std::vector<std::string> ids;
    for (const auto& tmpl : templates) {
        if (!TemplateMatchesWizardFilter(tmpl, m_WizardCategory)) {
            continue;
        }
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                tmpl.displayName + " " + tmpl.id + " " + tmpl.category + " "
                + tmpl.description + " " + JoinList(tmpl.tags));
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        ids.push_back(tmpl.id);
    }
    if (ids.empty()) {
        return;
    }
    int index = 0;
    for (int i = 0; i < static_cast<int>(ids.size()); ++i) {
        if (ids[static_cast<std::size_t>(i)] == m_WizardTemplateId) {
            index = i;
            break;
        }
    }
    index = std::clamp(index + delta, 0, static_cast<int>(ids.size()) - 1);
    m_WizardTemplateId = ids[static_cast<std::size_t>(index)];
    RebuildCreateWizard();
}

void LauncherShell::RebuildCreateWizard() {
    if (!m_ModalHost) {
        return;
    }
    const float s = LScale();
    m_ModalHost->SetVisible(true);
    m_ModalHost->SetDialogWidth(WizardDialogShell::kWidth);
    m_ModalHost->SetDialogHeight(WizardDialogShell::kHeight);

    const auto& templates = m_Context->Templates().Templates();
    const std::string query = ToLowerCopy(m_WizardTemplateQuery);

    std::vector<const ProjectTemplateInfo*> visible;
    visible.reserve(templates.size());
    for (const auto& tmpl : templates) {
        if (!TemplateMatchesWizardFilter(tmpl, m_WizardCategory)) {
            continue;
        }
        if (!query.empty()) {
            const std::string hay = ToLowerCopy(
                tmpl.displayName + " " + tmpl.id + " " + tmpl.category + " "
                + tmpl.description + " " + JoinList(tmpl.tags));
            if (hay.find(query) == std::string::npos) {
                continue;
            }
        }
        visible.push_back(&tmpl);
    }

    if (!visible.empty()) {
        bool stillVisible = false;
        for (const auto* tmpl : visible) {
            if (tmpl->id == m_WizardTemplateId) {
                stillVisible = true;
                break;
            }
        }
        if (!stillVisible) {
            m_WizardTemplateId = visible.front()->id;
        }
    }

    const ProjectTemplateInfo* selected = FindTemplateById(templates, m_WizardTemplateId);
    const std::string diskEstimate = selected
        ? EstimateTemplateDiskUsage(*selected)
        : "â€”";
    const std::string projectPathPreview = m_WizardLocation.empty()
        ? "â€”"
        : (m_WizardLocation + (m_WizardLocation.back() == '/' || m_WizardLocation.back() == '\\' ? "" : "/")
            + (m_WizardName.empty() ? "MyProject" : m_WizardName));

    auto shell = std::make_shared<WizardDialogShell>();

    auto root = std::make_shared<Column>();
    root->Gap(0.0f);
    root->SetHorizontalAlignment(HorizontalAlignment::Fill);
    root->SetVerticalAlignment(VerticalAlignment::Fill);
    root->Padding(Margin{ 24.0f * s, 24.0f * s, 24.0f * s, 24.0f * s });

    // Title
    auto title = MakeLabel("New project", 30.0f * s, LColor(ColorToken::TextPrimary));
    title->SetHorizontalAlignment(HorizontalAlignment::Left);
    root->AddChild(title);
    root->AddChild(std::make_shared<FixedGap>(1.0f, 12.0f * s));

    // Horizontal filter chips
    auto chips = std::make_shared<Row>();
    chips->Gap(8.0f * s);
    chips->SetVerticalAlignment(VerticalAlignment::Center);
    static const char* kFilters[] = { "All", "Games", "3D", "VR", "XR", "Samples" };
    for (const char* filter : kFilters) {
        const bool active = m_WizardCategory == filter;
        auto chip = std::make_shared<FilterChip>(filter, active);
        chip->SetOnClicked([this, filter = std::string(filter)] {
            m_WizardCategory = filter;
            RebuildCreateWizard();
        });
        chips->AddChild(chip);
    }
    root->AddChild(chips);
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));
    root->AddChild(std::make_shared<WizardSeparator>());
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));

    // Main two columns
    auto body = std::make_shared<Row>();
    body->Gap(0.0f);
    body->SetHorizontalAlignment(HorizontalAlignment::Fill);
    body->SetVerticalAlignment(VerticalAlignment::Fill);

    // LEFT â€” template list (~420)
    auto left = std::make_shared<Column>();
    left->Gap(12.0f * s);
    left->SetHorizontalAlignment(HorizontalAlignment::Fill);
    left->SetVerticalAlignment(VerticalAlignment::Fill);
    left->AddChild(std::make_shared<FixedGap>(420.0f * s, 1.0f));

    auto search = std::make_shared<CompactSearchField>("Search Templates");
    search->SetText(m_WizardTemplateQuery);
    search->SetOnChanged([this](const std::string& text) {
        if (m_WizardTemplateQuery == text) {
            return;
        }
        m_WizardTemplateQuery = text;
        RebuildCreateWizard();
    });
    left->AddChild(search);

    auto listScroll = std::make_shared<ScrollLayout>();
    auto list = std::make_shared<Column>();
    list->Gap(4.0f * s);
    list->SetHorizontalAlignment(HorizontalAlignment::Fill);

    if (visible.empty()) {
        list->AddChild(MakeLabel(
            "No matching templates",
            13.0f * s,
            LColor(ColorToken::TextMuted)));
    } else {
        for (const auto* tmpl : visible) {
            const bool isSelected = selected && tmpl->id == selected->id;
            auto row = std::make_shared<CreateTemplateRow>(*tmpl, isSelected);
            row->SetOnSelect([this, id = tmpl->id] {
                m_WizardTemplateId = id;
                m_Context->Settings().Settings().defaultTemplateId = id;
                RebuildCreateWizard();
            });
            row->SetOnActivate([this, id = tmpl->id] {
                m_WizardTemplateId = id;
                CommitCreateProject();
            });
            list->AddChild(row);
        }
    }
    listScroll->SetContent(list);
    listScroll->SetHorizontalAlignment(HorizontalAlignment::Fill);
    listScroll->SetVerticalAlignment(VerticalAlignment::Fill);
    left->AddChild(listScroll);
    body->AddChild(left);

    // Vertical separator
    auto vlineHost = std::make_shared<Column>();
    vlineHost->Padding(Margin{ 16.0f * s, 0.0f, 16.0f * s, 0.0f });
    vlineHost->SetVerticalAlignment(VerticalAlignment::Fill);
    auto vline = std::make_shared<Column>();
    vline->Background(LColor(ColorToken::Separator));
    vline->SetVerticalAlignment(VerticalAlignment::Fill);
    vline->AddChild(std::make_shared<FixedGap>(1.0f * s, 1.0f));
    vlineHost->AddChild(vline);
    body->AddChild(vlineHost);

    // RIGHT â€” details + settings (fixed, no nested scroll)
    auto right = std::make_shared<Column>();
    right->Gap(12.0f * s);
    right->SetHorizontalAlignment(HorizontalAlignment::Fill);
    right->SetVerticalAlignment(VerticalAlignment::Fill);

    right->AddChild(MakeLabel(
        "Selected Template",
        16.0f * s,
        LColor(ColorToken::TextPrimary)));

    if (selected) {
        right->AddChild(MakeLabel(
            selected->displayName,
            18.0f * s,
            LColor(ColorToken::TextPrimary)));
        right->AddChild(MakeLabel(
            selected->description.empty() ? "No description." : selected->description,
            12.0f * s,
            LColor(ColorToken::TextMuted)));

        auto addMeta = [&](const char* label, const std::string& value) {
            auto row = std::make_shared<Row>();
            row->Gap(8.0f * s);
            row->SetVerticalAlignment(VerticalAlignment::Center);
            auto l = MakeLabel(label, 13.0f * s, LColor(ColorToken::TextSecondary));
            l->SetHorizontalAlignment(HorizontalAlignment::Left);
            row->AddChild(l);
            row->AddChild(std::make_shared<Spacer>());
            auto v = MakeLabel(value.empty() ? "â€”" : value, 13.0f * s, LColor(ColorToken::TextPrimary));
            v->SetHorizontalAlignment(HorizontalAlignment::Right);
            row->AddChild(v);
            right->AddChild(row);
        };

        std::string engineCompat = "Any WindEffects install";
        if (m_Context->Engines().HasEngine()) {
            const auto& eng = m_Context->Engines().Current();
            engineCompat = eng.engineVersion.empty() ? "Local engine" : eng.engineVersion;
        }
        addMeta("Engine compatibility", engineCompat);
        addMeta("Required plugins", selected->plugins.empty() ? "None" : JoinList(selected->plugins));
        addMeta("Estimated disk usage", diskEstimate);
        addMeta("Recommended hardware", TemplateHardwareHint(*selected));
        addMeta("Template version", "1.0.0");
        addMeta("Template size", diskEstimate);
    } else {
        right->AddChild(MakeLabel(
            "Select a template from the list.",
            13.0f * s,
            LColor(ColorToken::TextMuted)));
    }

    right->AddChild(std::make_shared<FixedGap>(1.0f, 8.0f * s));
    right->AddChild(std::make_shared<WizardSeparator>());
    right->AddChild(MakeLabel(
        "Project Settings",
        16.0f * s,
        LColor(ColorToken::TextPrimary)));

    right->AddChild(MakeLabel("Project Name", 13.0f * s, LColor(ColorToken::TextSecondary)));
    auto nameBox = std::make_shared<TextBox>(m_WizardName, [this](const std::string& text) {
        m_WizardName = text;
    });
    right->AddChild(nameBox);

    right->AddChild(MakeLabel("Project Location", 13.0f * s, LColor(ColorToken::TextSecondary)));
    auto location = std::make_shared<PathPickerField>(m_WizardLocation, true);
    location->SetDialogTitle("Select Project Location");
    location->SetOnChanged([this](const std::string& path) {
        m_WizardLocation = path;
        RebuildCreateWizard();
    });
    right->AddChild(location);

    right->AddChild(MakeLabel("Engine Version", 13.0f * s, LColor(ColorToken::TextSecondary)));
    std::vector<std::string> engineLabels{ "Auto-detect" };
    std::vector<std::string> engineRoots{ {} };
    int engineIndex = 0;
    const auto& settings = m_Context->Settings().Settings();
    for (const auto& install : m_Context->Engines().InstalledEngines()) {
        const std::string version = install.engineVersion.empty() ? "Unknown" : install.engineVersion;
        engineLabels.push_back(version);
        engineRoots.push_back(install.engineRoot);
        if (install.engineRoot == settings.selectedEngineRoot) {
            engineIndex = static_cast<int>(engineRoots.size()) - 1;
        }
    }
    auto engineDrop = std::make_shared<SettingsDropdown>(engineLabels, engineIndex);
    engineDrop->SetOnChanged([this, engineRoots](int index) {
        if (index < 0 || index >= static_cast<int>(engineRoots.size())) {
            return;
        }
        m_Context->Settings().Settings().selectedEngineRoot = engineRoots[static_cast<std::size_t>(index)];
        PersistLauncherSettings("Engine version updated");
    });
    right->AddChild(engineDrop);

    right->AddChild(MakeLabel("Quality Preset", 13.0f * s, LColor(ColorToken::TextSecondary)));
    static const char* kQuality[] = { "Performance", "Balanced", "High Quality", "Ultra" };
    int qualityIndex = 1;
    for (int i = 0; i < 4; ++i) {
        if (m_WizardQuality == kQuality[i]) {
            qualityIndex = i;
            break;
        }
    }
    auto qualityDrop = std::make_shared<SettingsDropdown>(
        std::vector<std::string>{ kQuality[0], kQuality[1], kQuality[2], kQuality[3] },
        qualityIndex);
    qualityDrop->SetOnChanged([this](int index) {
        static const char* kQ[] = { "Performance", "Balanced", "High Quality", "Ultra" };
        if (index >= 0 && index < 4) {
            m_WizardQuality = kQ[index];
            m_Context->Settings().Settings().qualityPreset = m_WizardQuality;
        }
    });
    right->AddChild(qualityDrop);

    auto rightFiller = std::make_shared<FixedGap>(1.0f, 1.0f);
    rightFiller->SetVerticalAlignment(VerticalAlignment::Fill);
    right->AddChild(rightFiller);
    body->AddChild(right);

    root->AddChild(body);
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));
    root->AddChild(std::make_shared<WizardSeparator>());
    root->AddChild(std::make_shared<FixedGap>(1.0f, 16.0f * s));

    // Footer
    auto footer = std::make_shared<Row>();
    footer->Gap(12.0f * s);
    footer->SetVerticalAlignment(VerticalAlignment::Center);

    auto footerLeft = std::make_shared<Column>();
    footerLeft->Gap(2.0f * s);
    footerLeft->AddChild(MakeLabel(
        "Estimated disk usage  Â·  " + diskEstimate,
        12.0f * s,
        LColor(ColorToken::TextMuted)));
    footerLeft->AddChild(MakeLabel(
        EllipsizePath(projectPathPreview, 64),
        12.0f * s,
        LColor(ColorToken::TextSecondary)));
    footer->AddChild(footerLeft);
    footer->AddChild(std::make_shared<Spacer>());

    auto cancel = MakeSecondaryAction("Cancel", "");
    cancel->SetOnClicked([this] { CloseModal(); });
    footer->AddChild(cancel);
    auto create = MakePrimaryAction("Create Project", Icons::PlusName);
    create->SetOnClicked([this] { CommitCreateProject(); });
    footer->AddChild(create);
    root->AddChild(footer);

    shell->SetContent(root);
    m_ModalHost->SetContent(shell);
    InvalidateUI();
}
} // namespace we::programs::welauncher
