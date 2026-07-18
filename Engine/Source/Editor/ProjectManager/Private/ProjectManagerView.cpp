#include "ProjectManager/ProjectManagerView.h"

#include "Projects/EngineContext.h"
#include "Projects/ProjectLifecycle.h"
#include "Projects/RecentProjectsStore.h"

#include "KindUI/Core/Widgets/DesignSystemControls.h"
#include "KindUI/Layout/Flex.h"
#include "KindUI/Layout/Spacer.h"
#include "KindUI/Theming/StyleRole.h"
#include "KindUI/Core/Types.h"
#include "KindUI/Theming/ThemeAccess.h"
#include "KindUI/Tokens/DesignToken.h"
#include "KindUI/Widgets/Label.h"
#include "Platform/PlatformSDK.h"

#include "Core/Logger.h"

#include <cstdlib>

namespace we::editor::projectmanager {
namespace {

using namespace we::runtime::kindui;
using we::projects::EngineContext;
using we::projects::ProjectLifecycle;
using we::projects::ProjectCreateRequest;
using we::projects::RecentProjectsStore;

std::filesystem::path DefaultProjectsRoot() {
#if defined(_WIN32)
    const char* profile = std::getenv("USERPROFILE");
    if (profile && profile[0] != '\0') {
        return std::filesystem::path(profile) / "WindEffects" / "Projects";
    }
#endif
    const char* home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / "WindEffects" / "Projects";
}

std::shared_ptr<Label> MakeMuted(const std::string& text) {
    auto label = std::make_shared<Label>(text, TypographyToken::Caption);
    return label;
}

std::shared_ptr<Widget> BuildRecentRow(
    IProjectManagerHost& host,
    const we::projects::RecentProjectEntry& entry,
    std::function<void()> onRefresh) {
    auto row = std::make_shared<Row>();
    row->Gap(8.0f);
    row->Padding(Margin{ 8.0f, 6.0f, 8.0f, 6.0f });
    row->SetHorizontalAlignment(HorizontalAlignment::Fill);

    auto info = std::make_shared<Column>();
    info->Gap(2.0f);
    info->SetFlexGrow(1.0f);
    info->AddChild(std::make_shared<Label>(
        entry.displayName.empty() ? entry.weprojPath : entry.displayName,
        TypographyToken::Body));
    info->AddChild(MakeMuted(entry.weprojPath));
    if (!entry.engineVersion.empty()) {
        info->AddChild(MakeMuted("Engine " + entry.engineVersion
            + (entry.lastOpenedUtc.empty() ? "" : "  ·  " + entry.lastOpenedUtc)));
    }
    row->AddChild(info);

    auto openBtn = std::make_shared<PrimaryButton>("Open");
    openBtn->SetOnClicked([&host, path = entry.weprojPath]() {
        host.OnOpenProject(path);
    });
    row->AddChild(openBtn);

    auto removeBtn = std::make_shared<GhostButton>("Remove");
    removeBtn->SetOnClicked([path = entry.weprojPath, onRefresh]() {
        RecentProjectsStore::Get().Remove(path);
        if (onRefresh) {
            onRefresh();
        }
    });
    row->AddChild(removeBtn);

    return row;
}

std::shared_ptr<Widget> BrowseForProject(IProjectManagerHost& host) {
    auto btn = std::make_shared<SecondaryButton>("Browse...");
    btn->SetOnClicked([&host]() {
        we::platform::FileDialogDesc desc{};
        desc.mode = we::platform::FileDialogMode::OpenFile;
        desc.title = "Open WindEffects Project";
        desc.filters = {
            { "WindEffects Project", "*.weproj" },
            { "All Files", "*.*" },
        };
        const auto files = we::platform::Platform::Get().ShowFileDialog(desc);
        if (!files.empty()) {
            host.OnOpenProject(files.front());
        }
    });
    return btn;
}

std::shared_ptr<Widget> BuildNewProjectPanel(IProjectManagerHost& host) {
    auto panel = std::make_shared<Column>();
    panel->Gap(10.0f);
    panel->Padding(Margin{ 12.0f, 12.0f, 12.0f, 12.0f });
    panel->SetHorizontalAlignment(HorizontalAlignment::Fill);

    panel->AddChild(std::make_shared<SectionHeader>(
        "New Project",
        "Creates Config/, Content/, Source/, Plugins/, Saved/, Intermediate/, and a .weproj"));

    auto nameBox = std::make_shared<SearchBoxControl>("Project name");
    nameBox->SetText("MyGame");
    panel->AddChild(nameBox);

    auto locationLabel = std::make_shared<Label>(
        "Location: " + DefaultProjectsRoot().string(),
        TypographyToken::Caption);
    panel->AddChild(locationLabel);

    std::shared_ptr<std::filesystem::path> location =
        std::make_shared<std::filesystem::path>(DefaultProjectsRoot());

    auto pickLocation = std::make_shared<SecondaryButton>("Choose Folder...");
    pickLocation->SetOnClicked([location, locationLabel]() {
        we::platform::FileDialogDesc desc{};
        desc.mode = we::platform::FileDialogMode::OpenFolder;
        desc.title = "Project Parent Folder";
        const auto folders = we::platform::Platform::Get().ShowFileDialog(desc);
        if (!folders.empty()) {
            *location = folders.front();
            locationLabel->SetText("Location: " + location->string());
        }
    });
    panel->AddChild(pickLocation);

    auto engineLabel = std::make_shared<Label>(
        "Engine: " + EngineContext::Get().EngineVersion()
            + "  (" + EngineContext::Get().EngineRoot().string() + ")",
        TypographyToken::Caption);
    panel->AddChild(engineLabel);

    auto createBtn = std::make_shared<PrimaryButton>("Create Project");
    createBtn->SetOnClicked([&host, nameBox, location]() {
        ProjectCreateRequest request{};
        request.displayName = nameBox->GetText().empty() ? "MyGame" : nameBox->GetText();
        request.templateId = "Blank";
        request.parentDirectory = *location;
        request.engineVersion = EngineContext::Get().EngineVersion();
        request.engineRoot = EngineContext::Get().EngineRoot().string();

        std::error_code ec;
        std::filesystem::create_directories(request.parentDirectory, ec);

        const auto result = ProjectLifecycle::CreateNewProject(request);
        if (!result.success) {
            host.OnStatusMessage(result.message);
            HE_ERROR("[ProjectManager] " + result.message);
            return;
        }
        host.OnStatusMessage(result.message);
        host.OnOpenProject(result.weprojPath);
    });
    panel->AddChild(createBtn);

    return panel;
}

} // namespace

std::shared_ptr<Widget> ProjectManagerView::Build(
    IProjectManagerHost& host,
    const ProjectManagerViewOptions& options) {
    RecentProjectsStore::Get().Load();

    auto root = std::make_shared<Column>();
    root->Gap(0.0f);
    root->SetHorizontalAlignment(HorizontalAlignment::Fill);
    root->SetVerticalAlignment(VerticalAlignment::Fill);
    root->Background(ResolveColor(ColorToken::PanelContentBackground));

    auto header = std::make_shared<Column>();
    header->Gap(6.0f);
    header->Padding(Margin{ 24.0f, 20.0f, 24.0f, 12.0f });
    header->AddChild(std::make_shared<Label>("WindEffects Project Manager", TypographyToken::Title));
    header->AddChild(MakeMuted(
        "Select a project to edit. The Editor never assumes a default project."));
    header->AddChild(MakeMuted(
        "Engine " + EngineContext::Get().EngineVersion()
        + "  ·  " + EngineContext::Get().EngineRoot().string()));
    root->AddChild(header);

    auto actions = std::make_shared<Row>();
    actions->Gap(8.0f);
    actions->Padding(Margin{ 24.0f, 0.0f, 24.0f, 12.0f });

    auto bodyHost = std::make_shared<Column>();
    bodyHost->Gap(8.0f);
    bodyHost->Padding(Margin{ 24.0f, 8.0f, 24.0f, 24.0f });
    bodyHost->SetFlexGrow(1.0f);
    bodyHost->SetHorizontalAlignment(HorizontalAlignment::Fill);
    bodyHost->SetVerticalAlignment(VerticalAlignment::Fill);

    auto rebuildRecent = std::shared_ptr<std::function<void()>>(
        new std::function<void()>());

    *rebuildRecent = [&host, bodyHost, rebuildRecent]() {
        bodyHost->ClearChildren();
        bodyHost->AddChild(std::make_shared<SectionHeader>(
            "Recent Projects",
            "Open, clone, or remove entries from the recent list"));

        RecentProjectsStore::Get().Load();
        const auto& entries = RecentProjectsStore::Get().Entries();
        if (entries.empty()) {
            bodyHost->AddChild(MakeMuted("No recent projects. Create a new project or browse for a .weproj file."));
        } else {
            for (const auto& entry : entries) {
                bodyHost->AddChild(BuildRecentRow(host, entry, [rebuildRecent]() {
                    if (rebuildRecent && *rebuildRecent) {
                        (*rebuildRecent)();
                    }
                }));
            }
        }
    };

    auto showRecent = std::make_shared<SecondaryButton>("Recent");
    showRecent->SetOnClicked([rebuildRecent]() {
        if (rebuildRecent && *rebuildRecent) {
            (*rebuildRecent)();
        }
    });
    actions->AddChild(showRecent);

    auto newBtn = std::make_shared<PrimaryButton>("New Project");
    newBtn->SetOnClicked([&host, bodyHost]() {
        bodyHost->ClearChildren();
        bodyHost->AddChild(BuildNewProjectPanel(host));
    });
    actions->AddChild(newBtn);

    actions->AddChild(BrowseForProject(host));

    auto openExisting = std::make_shared<SecondaryButton>("Open Existing...");
    openExisting->SetOnClicked([&host]() {
        we::platform::FileDialogDesc desc{};
        desc.mode = we::platform::FileDialogMode::OpenFile;
        desc.title = "Open Existing Project";
        desc.filters = {
            { "WindEffects Project", "*.weproj" },
            { "All Files", "*.*" },
        };
        const auto files = we::platform::Platform::Get().ShowFileDialog(desc);
        if (!files.empty()) {
            const auto validation = ProjectLifecycle::ValidateProjectPath(
                files.front(),
                EngineContext::Get().EngineVersion());
            if (!validation.ok && !validation.needsUpgrade) {
                host.OnStatusMessage(validation.message);
                HE_ERROR("[ProjectManager] " + validation.message);
                return;
            }
            if (validation.needsUpgrade) {
                const auto upgrade = ProjectLifecycle::UpgradeProject(
                    files.front(),
                    EngineContext::Get().EngineVersion(),
                    EngineContext::Get().EngineRoot().string());
                host.OnStatusMessage(upgrade.message);
            }
            if (validation.missingSdk) {
                host.OnStatusMessage("Warning: " + validation.message);
            }
            host.OnOpenProject(files.front());
        }
    });
    actions->AddChild(openExisting);

    auto cloneBtn = std::make_shared<SecondaryButton>("Clone...");
    cloneBtn->SetOnClicked([&host, rebuildRecent]() {
        we::platform::FileDialogDesc desc{};
        desc.mode = we::platform::FileDialogMode::OpenFile;
        desc.title = "Clone Project";
        desc.filters = { { "WindEffects Project", "*.weproj" }, { "All Files", "*.*" } };
        const auto files = we::platform::Platform::Get().ShowFileDialog(desc);
        if (files.empty()) {
            return;
        }

        we::platform::FileDialogDesc folderDesc{};
        folderDesc.mode = we::platform::FileDialogMode::OpenFolder;
        folderDesc.title = "Clone Destination Folder";
        const auto folders = we::platform::Platform::Get().ShowFileDialog(folderDesc);
        if (folders.empty()) {
            return;
        }

        const auto sourceName = std::filesystem::path(files.front()).stem().string();
        const auto result = ProjectLifecycle::CloneProject(
            files.front(),
            sourceName + "_Clone",
            folders.front());
        host.OnStatusMessage(result.message);
        if (result.success) {
            RecentProjectsStore::Get().Touch(
                result.weprojPath,
                *ProjectLifecycle::ReadDescriptor(result.weprojPath));
            if (rebuildRecent && *rebuildRecent) {
                (*rebuildRecent)();
            }
            host.OnOpenProject(result.weprojPath);
        }
    });
    actions->AddChild(cloneBtn);

    root->AddChild(actions);
    root->AddChild(bodyHost);

    if (options.startInNewProjectMode) {
        bodyHost->ClearChildren();
        bodyHost->AddChild(BuildNewProjectPanel(host));
    } else {
        (*rebuildRecent)();
    }

    return root;
}

} // namespace we::editor::projectmanager
