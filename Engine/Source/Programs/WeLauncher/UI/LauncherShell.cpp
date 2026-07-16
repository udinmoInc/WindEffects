#include "UI/LauncherShell.h"

#include "Core/PaintContext.h"
#include "Widgets/Button.h"
#include "Widgets/Label.h"
#include "Widgets/Panel.h"
#include "Widgets/TextBox.h"
#include "Layout/Box.h"
#include "Layout/ScrollLayout.h"
#include "Layout/Spacer.h"
#include "WindEffects/Editor/UI/Theming/ThemeAccess.h"
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"

#include "Platform/PlatformSDK.h"
#include "Util/PathUtils.h"

using namespace WindEffects::Editor::UI;

namespace we::programs::welauncher {
namespace {

Color ThemeText() { return ResolveThemeColor(ThemeToken::TextPrimary); }
Color ThemeMuted() { return ResolveThemeColor(ThemeToken::TextSecondary); }

std::shared_ptr<Label> MakeLabel(const std::string& text, float size, Color color) {
    auto label = std::make_shared<Label>(text, color, size);
    label->SetHorizontalAlignment(HorizontalAlignment::Left);
    return label;
}

std::shared_ptr<Panel> MakeSection(const std::string& title, const std::shared_ptr<Widget>& content) {
    auto panel = std::make_shared<Panel>();
    panel->SetBackgroundColor(ResolveThemeColor(ThemeToken::PanelBackground));
    auto box = std::make_shared<VerticalBox>();
    box->SetPadding(Margin{12.0f, 12.0f, 12.0f, 12.0f});
    box->SetSpacing(8.0f);
    box->AddChild(MakeLabel(title, 14.0f, ThemeText()));
    box->AddChild(content);
    panel->SetContent(box);
    return panel;
}

} // namespace

LauncherShell::LauncherShell(std::shared_ptr<LauncherContext> context)
    : m_Context(std::move(context)) {
}

void LauncherShell::Construct(float dpiScale) {
    (void)dpiScale;
    m_WizardLocation = m_Context->Settings().Settings().defaultProjectsRoot;
    RefreshProjectList();

    auto root = std::make_shared<VerticalBox>();
    root->SetPadding(Margin{16.0f, 16.0f, 16.0f, 16.0f});
    root->SetSpacing(12.0f);

    auto headerRow = std::make_shared<HorizontalBox>();
    headerRow->SetSpacing(12.0f);
    headerRow->AddChild(MakeLabel("WindEffects Launcher", 22.0f, ThemeText()));
    headerRow->AddChild(std::make_shared<Spacer>());
    headerRow->AddChild(std::make_shared<Button>("Create Project", [this] { ShowCreateWizard(); }));
    headerRow->AddChild(std::make_shared<Button>("Browse...", [this] { BrowseForProject(); }));
    headerRow->AddChild(std::make_shared<Button>("Open", [this] { OpenSelectedProject(); }));
    root->AddChild(headerRow);

    m_StatusLabel = MakeLabel(m_Context->StatusMessage(), 12.0f, ThemeMuted());
    root->AddChild(m_StatusLabel);

    auto body = std::make_shared<HorizontalBox>();
    body->SetSpacing(12.0f);

    // Left: project list
    m_ProjectListPanel = std::make_shared<VerticalBox>();
    auto projectScroll = std::make_shared<ScrollLayout>();
    projectScroll->SetContent(m_ProjectListPanel);
    body->AddChild(MakeSection("Recent Projects", projectScroll));

    // Center: details + actions
    m_DetailsPanel = std::make_shared<VerticalBox>();
    body->AddChild(MakeSection("Project Details", m_DetailsPanel));

    // Right column: engine + sdk + future
    auto rightColumn = std::make_shared<VerticalBox>();
    rightColumn->SetSpacing(12.0f);

    m_EnginePanel = std::make_shared<VerticalBox>();
    rightColumn->AddChild(MakeSection("Engine", m_EnginePanel));

    m_SdkPanel = std::make_shared<VerticalBox>();
    rightColumn->AddChild(MakeSection("SDK Status", m_SdkPanel));

    m_FuturePanel = std::make_shared<VerticalBox>();
    rightColumn->AddChild(MakeSection("Coming Soon", m_FuturePanel));

    body->AddChild(rightColumn);
    root->AddChild(body);

    // Wizard overlay (hidden until requested)
    m_WizardOverlay = std::make_shared<VerticalBox>();
    m_WizardOverlay->SetVisible(false);
    root->AddChild(m_WizardOverlay);

    m_Root = root;
    SetStatus(m_Context->StatusMessage());
}

void LauncherShell::RefreshProjectList() {
    m_Projects = m_Context->Projects().LoadRecentSummaries();
    if (m_ProjectListPanel) {
        m_ProjectListPanel->ClearChildren();
        if (m_Projects.empty()) {
            m_ProjectListPanel->AddChild(MakeLabel("No recent projects. Create or browse to get started.", 12.0f, ThemeMuted()));
        } else {
            for (std::size_t i = 0; i < m_Projects.size(); ++i) {
                const auto& project = m_Projects[i];
                const std::string label = project.descriptor.displayName + "  (" + project.descriptor.templateId + ")";
                m_ProjectListPanel->AddChild(std::make_shared<Button>(label, [this, i] { SelectProject(i); }));
            }
        }
    }

    if (m_EnginePanel) {
        m_EnginePanel->ClearChildren();
        if (m_Context->Engines().HasEngine()) {
            const auto& engine = m_Context->Engines().Current();
            m_EnginePanel->AddChild(MakeLabel("Version: " + engine.engineVersion, 12.0f, ThemeText()));
            m_EnginePanel->AddChild(MakeLabel("Root: " + PathUtils::ToUtf8(engine.engineRoot), 11.0f, ThemeMuted()));
            for (const auto& install : m_Context->Engines().InstalledEngines()) {
                const std::string prefix = install.isCurrent ? "* " : "  ";
                m_EnginePanel->AddChild(MakeLabel(prefix + install.displayLabel, 11.0f, ThemeMuted()));
            }
        } else {
            m_EnginePanel->AddChild(MakeLabel("No engine detected.", 12.0f, ThemeMuted()));
        }
    }

    if (m_SdkPanel) {
        m_SdkPanel->ClearChildren();
        for (const auto& check : m_Context->Sdk().RunChecks()) {
            std::string status = check.name + ": ";
            switch (check.status) {
            case SdkCheckStatus::Pass: status += "OK — "; break;
            case SdkCheckStatus::Warn: status += "WARN — "; break;
            case SdkCheckStatus::Fail: status += "FAIL — "; break;
            }
            status += check.detail;
            m_SdkPanel->AddChild(MakeLabel(status, 11.0f, ThemeMuted()));
        }
    }

    if (m_FuturePanel) {
        m_FuturePanel->ClearChildren();
        const char* features[] = {
            "Plugin Management",
            "Engine Updates",
            "Marketplace",
            "Sample Projects",
            "Asset Downloads",
            "Project Migration",
            "Repair Tools",
            "Cache Management",
        };
        for (const char* feature : features) {
            auto row = std::make_shared<HorizontalBox>();
            row->AddChild(MakeLabel(feature, 11.0f, ThemeMuted()));
            row->AddChild(std::make_shared<Spacer>());
            row->AddChild(MakeLabel("Soon", 11.0f, ThemeMuted()));
            m_FuturePanel->AddChild(row);
        }
    }

    SelectProject(m_SelectedIndex >= 0 ? static_cast<std::size_t>(m_SelectedIndex) : 0);
}

void LauncherShell::SetStatus(const std::string& message) {
    m_Context->SetStatusMessage(message);
    if (m_StatusLabel) {
        auto label = std::dynamic_pointer_cast<Label>(m_StatusLabel);
        if (label) {
            label->SetText(message);
        }
    }
}

void LauncherShell::SelectProject(std::size_t index) {
    if (m_Projects.empty()) {
        m_SelectedIndex = -1;
        if (m_DetailsPanel) {
            m_DetailsPanel->ClearChildren();
            m_DetailsPanel->AddChild(MakeLabel("Select a project to view details.", 12.0f, ThemeMuted()));
        }
        return;
    }
    if (index >= m_Projects.size()) {
        index = 0;
    }
    m_SelectedIndex = static_cast<int>(index);
    const auto& project = m_Projects[index];

    if (m_DetailsPanel) {
        m_DetailsPanel->ClearChildren();
        m_DetailsPanel->AddChild(MakeLabel(project.descriptor.displayName, 16.0f, ThemeText()));
        m_DetailsPanel->AddChild(MakeLabel("Template: " + project.descriptor.templateId, 12.0f, ThemeMuted()));
        m_DetailsPanel->AddChild(MakeLabel("Engine: " + project.descriptor.engineVersion, 12.0f, ThemeMuted()));
        m_DetailsPanel->AddChild(MakeLabel(project.compatible ? project.compatibilityMessage : "Incompatible: " + project.compatibilityMessage,
            12.0f, project.compatible ? ThemeMuted() : Color{1.0f, 0.45f, 0.35f, 1.0f}));
        m_DetailsPanel->AddChild(MakeLabel(project.weprojPath, 11.0f, ThemeMuted()));

        auto actions = std::make_shared<HorizontalBox>();
        actions->SetSpacing(8.0f);
        actions->AddChild(std::make_shared<Button>("Launch Editor", [this] { OpenSelectedProject(); }));
        actions->AddChild(std::make_shared<Button>("Clone", [this] { CloneSelectedProject(); }));
        actions->AddChild(std::make_shared<Button>("Rename", [this] { RenameSelectedProject(); }));
        actions->AddChild(std::make_shared<Button>("Delete", [this] { DeleteSelectedProject(); }));
        m_DetailsPanel->AddChild(actions);
    }
}

void LauncherShell::ShowCreateWizard() {
    m_ShowWizard = true;
    if (!m_WizardOverlay) {
        return;
    }
    m_WizardOverlay->SetVisible(true);
    m_WizardOverlay->ClearChildren();

    auto box = std::make_shared<VerticalBox>();
    box->SetPadding(Margin{16.0f, 16.0f, 16.0f, 16.0f});
    box->SetSpacing(10.0f);
    box->AddChild(MakeLabel("Create New Project", 18.0f, ThemeText()));

    auto templateRow = std::make_shared<HorizontalBox>();
    templateRow->SetSpacing(6.0f);
    for (const auto& tmpl : m_Context->Templates().Templates()) {
        templateRow->AddChild(std::make_shared<Button>(tmpl.displayName, [this, id = tmpl.id] {
            m_WizardTemplateId = id;
            SetStatus("Selected template: " + id);
        }));
    }
    box->AddChild(templateRow);

    auto nameBox = std::make_shared<TextBox>(m_WizardName, [this](const std::string& text) {
        m_WizardName = text;
    });
    box->AddChild(MakeLabel("Project Name", 12.0f, ThemeMuted()));
    box->AddChild(nameBox);

    auto locationBox = std::make_shared<TextBox>(m_WizardLocation, [this](const std::string& text) {
        m_WizardLocation = text;
    });
    box->AddChild(MakeLabel("Location", 12.0f, ThemeMuted()));
    box->AddChild(locationBox);

    auto buttons = std::make_shared<HorizontalBox>();
    buttons->SetSpacing(8.0f);
    buttons->AddChild(std::make_shared<Button>("Cancel", [this] {
        m_ShowWizard = false;
        if (m_WizardOverlay) {
            m_WizardOverlay->SetVisible(false);
        }
    }));
    buttons->AddChild(std::make_shared<Spacer>());
    buttons->AddChild(std::make_shared<Button>("Create", [this] {
        const auto result = m_Context->Projects().CreateProject(
            m_WizardName,
            m_WizardTemplateId,
            PathUtils::FromUtf8(m_WizardLocation));
        if (!result.success) {
            SetStatus(result.message);
            return;
        }
        m_ShowWizard = false;
        if (m_WizardOverlay) {
            m_WizardOverlay->SetVisible(false);
        }
        SetStatus(result.message);
        RefreshProjectList();
    }));
    box->AddChild(buttons);

    auto panel = std::make_shared<Panel>();
    panel->SetBackgroundColor(ResolveThemeColor(ThemeToken::DialogBackground));
    panel->SetContent(box);
    m_WizardOverlay->AddChild(panel);
}

void LauncherShell::BrowseForProject() {
    auto& platform = we::platform::Platform::Get();
    we::platform::FileDialogDesc desc{};
    desc.mode = we::platform::FileDialogMode::OpenFile;
    desc.title = "Open WindEffects Project";
    desc.filters = { { "WindEffects Project", "*.weproj" }, { "All Files", "*.*" } };

    const auto paths = platform.ShowFileDialog(desc);
    if (paths.empty()) {
        return;
    }

    const auto launch = m_Context->EditorLaunch().Launch(PathUtils::FromUtf8(paths[0]));
    SetStatus(launch.message);
    RefreshProjectList();
}

void LauncherShell::OpenSelectedProject() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        SetStatus("No project selected.");
        return;
    }
    const auto launch = m_Context->EditorLaunch().Launch(PathUtils::FromUtf8(m_Projects[static_cast<std::size_t>(m_SelectedIndex)].weprojPath));
    SetStatus(launch.message);
    RefreshProjectList();
}

void LauncherShell::CloneSelectedProject() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        return;
    }
    const auto& selected = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];
    const auto result = m_Context->Projects().CloneProject(
        PathUtils::FromUtf8(selected.weprojPath),
        selected.descriptor.displayName + "_Copy");
    SetStatus(result.message);
    RefreshProjectList();
}

void LauncherShell::RenameSelectedProject() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        return;
    }
    const auto& selected = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];
    const auto result = m_Context->Projects().RenameProject(
        PathUtils::FromUtf8(selected.weprojPath),
        selected.descriptor.displayName + "_Renamed");
    SetStatus(result.message);
    RefreshProjectList();
}

void LauncherShell::DeleteSelectedProject() {
    if (m_SelectedIndex < 0 || m_SelectedIndex >= static_cast<int>(m_Projects.size())) {
        return;
    }
    const auto& selected = m_Projects[static_cast<std::size_t>(m_SelectedIndex)];

    auto& platform = we::platform::Platform::Get();
    const auto confirm = platform.ShowMessageBox({
        .title = "Delete Project",
        .message = ("Delete project and all files?\n" + selected.weprojPath).c_str(),
        .type = we::platform::MessageBoxType::Warning,
        .yesNo = true,
    });
    if (confirm != we::platform::MessageBoxResult::Yes) {
        return;
    }

    const auto result = m_Context->Projects().DeleteProject(PathUtils::FromUtf8(selected.weprojPath));
    SetStatus(result.message);
    m_SelectedIndex = -1;
    RefreshProjectList();
}

Size LauncherShell::Measure(const Size& availableSize) {
    if (m_Root) {
        m_Root->Measure(availableSize);
    }
    return availableSize;
}

void LauncherShell::Arrange(const Rect& allottedRect) {
    if (m_Root) {
        m_Root->Arrange(allottedRect);
    }
}

void LauncherShell::Paint(PaintContext& context) {
    context.FillRect(GetGeometry(), ResolveThemeColor(ThemeToken::WorkspaceBackground));
    if (m_Root) {
        m_Root->Paint(context);
    }
}

void LauncherShell::Tick(float deltaTime) {
    Widget::Tick(deltaTime);
    if (m_Root) {
        m_Root->Tick(deltaTime);
    }
}

} // namespace we::programs::welauncher
