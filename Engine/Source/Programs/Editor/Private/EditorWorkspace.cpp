// ==============================================================================
// WindEffects — Editor — EditorWorkspace
// Internal implementation for the Editor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Editor.h"
#include "EditorShellBuilder.h"
#include "EditorWindowHitTest.h"
#include "ContentBrowser/ContentBrowserApi.h"
#include "ContentBrowser/ContentBrowserRuntime.h"
#include "Compilation/Compilation.h"
#include "Core/Logger.h"
#include "Core/PluginManager.h"
#include "DefaultScene/DefaultSceneBuilder.h"
#include "Environment/EnvironmentSystem.h"
#include "Explorer/WorldOutlinerApi.h"
#include "KindUI/Core/DPIContext.h"
#include "KindUI/Layout/OverlayManager.h"
#include "Platform/PlatformSDK.h"
#include "Prefab/Prefab.h"
#include "PrefabEditor/PrefabEditor.h"
#include "Projects/EngineContext.h"
#include "Projects/ProjectContext.h"
#include "Projects/ProjectLifecycle.h"
#include "PropertyEditor/IPropertyEditorRuntime.h"
#include "PropertyEditor/PropertyEditorSession.h"
#include "Reflection/ITypeRegistry.h"
#include "Serialization/ISerializer.h"
#include "Terrain/Terrain.h"
#include "TerrainEditor/TerrainEditor.h"
#include "Undo/UndoTypes.h"
#include "ViewportEdit/ViewportEdit.h"
#include "Widgets/ViewportWidget.h"
#include "WindEffects/Editor/UI/Shell/EditorWorkspaceController.h"
#include "WorldOutliner/WorldOutliner.h"

#include <algorithm>

#include "Platform/UndefWin32Macros.h"

namespace we::programs::editor {

using ::we::editor::viewport::ViewportWidget;
using ::we::runtime::world::DefaultSceneBuilder;

bool Editor::LaunchWeLauncher(const std::vector<std::string>& extraArgs) {
    HE_WARN("[Startup] WeLauncher launch blocked by user request.");
    return false;
}

void Editor::OpenWeLauncher() {
    if (!LaunchWeLauncher()) {
        m_StatusMessage = "WeLauncher.exe not found. Build the WeLauncher target.";
        HE_ERROR("[Startup] " + m_StatusMessage);
    }
}

void Editor::UnloadProjectWorkspace() {
    HE_INFO("[Startup] Unloading current project workspace...");

    if (m_Window != we::platform::WindowId::Invalid) {
        we::platform::Platform::Get().SetWindowHitTest(m_Window, nullptr, nullptr);
    }
    m_WindowHitTestData.titleBar.reset();

    if (m_OverlayHost) {
        m_OverlayHost->CloseAllPopups();
    }
    EditorWorkspaceController::Get().SaveLayout();
    EditorWorkspaceController::Get().Reset();

    we::core::PluginManager::Get().UnloadAllPlugins();
    ShutdownContentBrowserService();
    m_ContentBrowser.reset();

    if (m_Scene) {
        m_Scene->Clear();
    }

    m_ViewportWidget.reset();
    m_TitleBar.reset();
    m_StatusBar.reset();
    m_OverlayHost.reset();
    m_RootWidget.reset();
    EditorWorkspaceController::Get().ClearLayoutRefs();

    if (m_UndoRuntime) {
        m_UndoRuntime->Shutdown();
        m_UndoRuntime.reset();
    }
    m_ViewportEdit.reset();
    ::we::editor::viewportedit::ViewportEditSession::Clear();
    if (m_WorldOutliner) {
        m_WorldOutliner->Shutdown();
        m_WorldOutliner.reset();
    }
    ::we::editor::outliner::WorldOutlinerSession::Clear();
    m_ContentBrowser.reset();
    ::we::editor::contentbrowser::ContentBrowserSession::Clear();
    if (m_PrefabEditor) {
        m_PrefabEditor->Shutdown();
        m_PrefabEditor.reset();
    }
    ::we::editor::prefab::PrefabSession::Clear();
    if (m_PrefabRuntime) {
        m_PrefabRuntime->Shutdown();
        m_PrefabRuntime.reset();
    }
    if (m_Renderer) {
        m_Renderer->ClearTerrainDrawer();
    }
    if (m_TerrainRuntime) {
        m_TerrainRuntime->Shutdown();
        m_TerrainRuntime.reset();
        we::runtime::terrain::SetDefaultTerrainRuntime(nullptr);
    }
    if (m_CompilationRuntime) {
        m_CompilationRuntime->Shutdown();
        m_CompilationRuntime.reset();
    }
    ::we::runtime::compilation::CompilationSession::Clear();
    ::we::editor::property::PropertyEditorSession::Clear();
    m_Serializer.reset();

    if (we::projects::ProjectContext::Get().IsLoaded()) {
        we::projects::ProjectContext::Get().Unload();
    }
}

void Editor::EnterProjectWorkspace(const std::filesystem::path& weprojPath) {
    HE_INFO("[Startup] Loading project: " + weprojPath.string());

    if (we::projects::ProjectContext::Get().IsLoaded()
        || m_ViewportWidget
        || m_StatusBar) {
        UnloadProjectWorkspace();
    }

    auto validation = we::projects::ProjectLifecycle::ValidateProjectPath(
        weprojPath,
        we::projects::EngineContext::Get().EngineVersion());
    if (validation.needsUpgrade) {
        const auto upgrade = we::projects::ProjectLifecycle::UpgradeProject(
            weprojPath,
            we::projects::EngineContext::Get().EngineVersion(),
            we::projects::EngineContext::Get().EngineRoot().string());
        HE_INFO("[Startup] Project upgrade: " + upgrade.message);
        m_StatusMessage = upgrade.message;
        validation = we::projects::ProjectLifecycle::ValidateProjectPath(
            weprojPath,
            we::projects::EngineContext::Get().EngineVersion());
    }

    if (!validation.ok) {
        HE_WARN("[Startup] Project validation failed, but continuing anyway: " + validation.message);
        m_StatusMessage = validation.message;
    }
    if (validation.missingSdk) {
        HE_WARN("[Startup] Missing SDK / config warning: " + validation.message);
        m_StatusMessage = validation.message;
    }

    const auto loadResult = we::projects::ProjectContext::Get().Load(weprojPath);
    if (!loadResult.ok) {
        HE_WARN("[Startup] Failed to load ProjectContext, continuing anyway: " + loadResult.message);
        m_StatusMessage = loadResult.message;
    }

    auto& project = we::projects::ProjectContext::Get();

    // Project plugins (never a hardcoded path).
    if (!m_CommandLine.safeMode) {
        try {
            we::core::PluginManager::Get().ScanAndLoadPlugins(project.PluginsRoot().string());
        } catch (const std::exception& e) {
            HE_ERROR("[Startup] Failed to load project plugins: " + std::string(e.what()));
        }
    } else {
        HE_INFO("[Startup] Safe mode: skipping project plugins.");
    }

    // Editor world — empty default environment; startup map path recorded on context.
    if (m_Scene) {
        m_Scene->Clear();
    }
    DefaultSceneBuilder::CreateDefaultScene(*m_Scene);
    if (!project.Descriptor().startupMap.empty()) {
        project.SetCurrentMap(project.Descriptor().startupMap);
        HE_INFO("[Startup] Startup map from .weproj: " + project.Descriptor().startupMap
            + " (scene file load pipeline reserved)");
    }
    we::runtime::world::environment::EnvironmentSystem::Get().UpdateRendering(m_Camera->GetPosition());

    HE_INFO("[Startup] Building editor shell for project...");
    try {
        we::editor::undo::UndoDependencies undoDeps;
        undoDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        m_UndoRuntime = we::editor::undo::CreateUndoRuntime(std::move(undoDeps));

        m_Serializer = we::runtime::serialization::CreateSerializer({});

        we::editor::property::PropertyEditorDependencies peDeps;
        peDeps.typeRegistry = &we::runtime::reflection::GetTypeRegistry();
        peDeps.serializer = m_Serializer.get();
        if (m_UndoRuntime) {
            peDeps.transactionHook = m_UndoRuntime->MakePropertyTransactionHook();
        }
        peDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        auto peRuntime = we::editor::property::CreatePropertyEditorRuntime(std::move(peDeps));
        auto detailsView = peRuntime ? peRuntime->MakeDetailsView() : nullptr;
        we::editor::property::PropertyEditorSession::Install(
            std::shared_ptr<we::editor::property::IPropertyEditorRuntime>(std::move(peRuntime)),
            std::shared_ptr<we::editor::property::IDetailsView>(std::move(detailsView)));

        we::editor::viewportedit::ViewportEditDependencies veDeps;
        veDeps.undo = m_UndoRuntime.get();
        veDeps.propertyEditor = we::editor::property::PropertyEditorSession::Runtime();
        veDeps.scene = m_Scene.get();
        veDeps.editorCamera = m_Camera.get();
        m_ViewportEdit = we::editor::viewportedit::CreateViewportEditRuntime(veDeps);
        we::editor::viewportedit::ViewportEditSession::Install(m_ViewportEdit);

        {
            we::runtime::terrain::TerrainDependencies terrainDeps;
            terrainDeps.typeRegistry = &we::runtime::reflection::GetTypeRegistry();
            terrainDeps.serializer = m_Serializer.get();
            terrainDeps.scene = m_Scene.get();
            if (m_Renderer) {
                terrainDeps.device = m_Renderer->GetRHIDevice();
            }
            terrainDeps.onLog = [](std::string_view msg) {
                HE_INFO(std::string(msg));
            };
            auto terrainRuntime = we::runtime::terrain::CreateTerrainRuntime(terrainDeps);
            we::runtime::terrain::SetDefaultTerrainRuntime(
                std::unique_ptr<we::runtime::terrain::ITerrainRuntime>(
                    terrainRuntime.release()));
            m_TerrainRuntime = std::shared_ptr<we::runtime::terrain::ITerrainRuntime>(
                &we::runtime::terrain::GetDefaultTerrainRuntime(),
                [](we::runtime::terrain::ITerrainRuntime*) {});

            auto& landscape = we::editor::terrain::GetLandscapeEditor();
            landscape.BindTerrainRuntime(m_TerrainRuntime.get());
            landscape.BindScene(m_Scene.get());
            landscape.BindUndo(m_UndoRuntime.get());
            landscape.BindViewport(m_ViewportEdit.get());
            landscape.InstallViewportMode();

            if (m_Renderer) {
                we::runtime::terrain::TerrainSystem::Get().BindRenderer(m_Renderer->GetRHIDevice());
                m_Renderer->SetTerrainDrawer(
                    [](we::rhi::IRHICommandList& cmd,
                        we::rhi::RHITextureHandle color,
                        we::rhi::RHITextureHandle depth,
                        we::rhi::Extent2D extent,
                        const we::runtime::renderer::CameraUniform& camera,
                        const we::runtime::renderer::SceneEnvironmentUniform& environment) {
                        we::runtime::terrain::TerrainSceneLighting lighting{};
                        lighting.sunDirection = environment.sunDirection;
                        lighting.sunIntensity = environment.sunIntensity;
                        lighting.sunColor = environment.sunColor;
                        lighting.skyLightIntensity = environment.skyLightIntensity;
                        lighting.skyAmbientColor = environment.skyAmbientColor;
                        lighting.skyLightLowerColor = environment.skyLightLowerColor;
                        lighting.valid = true;

                        auto& runtime = we::runtime::terrain::GetDefaultTerrainRuntime();
                        for (const auto id : runtime.Manager().ListAll()) {
                            if (auto* terrain = runtime.Manager().Find(id)) {
                                // Tick owns remesh/upload; draw path only submits GPU.
                                terrain->Renderer().DrawViewport(
                                    cmd,
                                    color,
                                    depth,
                                    extent,
                                    camera.view,
                                    camera.proj,
                                    camera.position,
                                    &lighting);
                            }
                        }
                    });
            }
        }

        we::editor::outliner::WorldOutlinerDependencies woDeps;
        woDeps.undo = m_UndoRuntime.get();
        woDeps.propertyEditor = we::editor::property::PropertyEditorSession::Runtime();
        woDeps.detailsView = we::editor::property::PropertyEditorSession::Details();
        woDeps.viewportEdit = m_ViewportEdit.get();
        woDeps.scene = m_Scene.get();
        woDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        m_WorldOutliner = std::shared_ptr<we::editor::outliner::IWorldOutlinerRuntime>(
            we::editor::outliner::CreateWorldOutlinerRuntime(woDeps));
        we::editor::outliner::WorldOutlinerSession::Install(m_WorldOutliner);

        we::editor::contentbrowser::ContentBrowserDependencies cbDeps;
        cbDeps.iconRenderer = m_OverlayRenderer->GetIconRenderer();
        cbDeps.contentRoot = project.ContentRoot();
        if (m_UndoRuntime) {
            cbDeps.recordTransaction = [this](
                std::string_view label,
                std::function<bool()> undoFn,
                std::function<bool()> redoFn) {
                return m_UndoRuntime->Manager().RecordCustom(
                    label,
                    we::editor::undo::TransactionKind::Generic,
                    std::string(label),
                    std::move(undoFn),
                    std::move(redoFn));
            };
        }
        cbDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        m_ContentBrowser = std::shared_ptr<we::editor::contentbrowser::IContentBrowserRuntime>(
            we::editor::contentbrowser::CreateContentBrowserRuntime(cbDeps));
        we::editor::contentbrowser::ContentBrowserSession::Install(m_ContentBrowser);

        we::runtime::prefab::PrefabDependencies prefabDeps;
        prefabDeps.scene = m_Scene.get();
        prefabDeps.serializer = m_Serializer.get();
        prefabDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        m_PrefabRuntime = std::shared_ptr<we::runtime::prefab::IPrefabRuntime>(
            we::runtime::prefab::CreatePrefabRuntime(prefabDeps));

        we::editor::prefab::PrefabEditorDependencies prefabEditorDeps;
        prefabEditorDeps.prefabRuntime = m_PrefabRuntime.get();
        prefabEditorDeps.scene = m_Scene.get();
        prefabEditorDeps.viewportEdit = m_ViewportEdit.get();
        prefabEditorDeps.worldOutliner = m_WorldOutliner.get();
        prefabEditorDeps.contentBrowser = m_ContentBrowser.get();
        if (m_UndoRuntime) {
            prefabEditorDeps.recordTransaction = [this](
                std::string_view label,
                std::function<bool()> undoFn,
                std::function<bool()> redoFn) {
                return m_UndoRuntime->Manager().RecordCustom(
                    label,
                    we::editor::undo::TransactionKind::Prefab,
                    std::string(label),
                    std::move(undoFn),
                    std::move(redoFn));
            };
        }
        prefabEditorDeps.onLog = [](std::string_view msg) {
            HE_INFO(std::string(msg));
        };
        m_PrefabEditor = std::shared_ptr<we::editor::prefab::IPrefabEditor>(
            we::editor::prefab::CreatePrefabEditor(prefabEditorDeps));
        we::editor::prefab::PrefabSession::Install(m_PrefabEditor);

        {
            we::runtime::compilation::CompilationDependencies compileDeps;
            compileDeps.config.enableBackgroundCompilation = true;
            compileDeps.config.workerCount = 0;
            const auto ddcDir = project.ContentRoot() / "DerivedData" / "Compilation";
            std::error_code ec;
            std::filesystem::create_directories(ddcDir, ec);
            compileDeps.config.databasePath = (ddcDir / "wecomp.db").string();
            compileDeps.config.cacheDirectory = ddcDir.string();
            compileDeps.onLog = [](std::string_view msg) {
                HE_INFO(std::string(msg));
            };
            m_CompilationRuntime = std::shared_ptr<we::runtime::compilation::ICompilationRuntime>(
                we::runtime::compilation::CreateCompilationRuntime(compileDeps));
            m_CompilationRuntime->RegisterBuiltinCompilers();
            we::runtime::compilation::CompilationSession::Install(m_CompilationRuntime);
        }

        if (m_ViewportEdit && m_PrefabEditor) {
            m_ViewportEdit->DragDrop().SetExternalDropHandler(
                [this](
                    float,
                    float,
                    std::string_view payloadType,
                    std::string_view payloadData,
                    const we::editor::viewportedit::ViewportHit& hit) {
                    if (!m_PrefabEditor) {
                        return false;
                    }
                    if (payloadType != "prefab" && payloadType != "prefab-guid" && payloadType != "asset") {
                        return false;
                    }
                    const we::math::Vec3 pos = hit.valid ? hit.worldPoint : we::math::Vec3{};
                    return m_PrefabEditor->SpawnFromPayload(payloadType, payloadData, pos).IsValid();
                });
        }

        we::programs::editor::EditorShellDependencies shellDeps;
        shellDeps.window = m_Window;
        shellDeps.renderer = m_Renderer.get();
        shellDeps.scene = m_Scene;
        shellDeps.camera = m_Camera;
        shellDeps.overlayRenderer = m_OverlayRenderer.get();
        shellDeps.eventSystem = m_UIEventSystem;
        shellDeps.dpiScale = we::runtime::kindui::DPIContext::GetScale();
        shellDeps.onCreateNewLevel = [this]() { CreateNewLevel(); };
        shellDeps.onOpenProject = [this]() { OpenProjectDialog(); };
        shellDeps.onOpenProjectManager = [this]() { OpenWeLauncher(); };
        shellDeps.onUndo = [this]() {
            if (m_UndoRuntime) {
                (void)m_UndoRuntime->Manager().Undo();
            }
        };
        shellDeps.onRedo = [this]() {
            if (m_UndoRuntime) {
                (void)m_UndoRuntime->Manager().Redo();
            }
        };
        shellDeps.onViewportCreated = [this](std::shared_ptr<we::runtime::kindui::Widget>& viewportWidget) {
            m_ViewportWidget = viewportWidget;
            if (auto vp = std::dynamic_pointer_cast<ViewportWidget>(viewportWidget)) {
                vp->SetEditInputHandler([this, vp](const we::runtime::kindui::MouseEvent& event, float localX,
                    float localY) {
                    if (!m_ViewportEdit) {
                        return false;
                    }
                    const auto geom = vp->GetGeometry();
                    m_ViewportEdit->SetViewportSize(std::max(1.f, geom.width), std::max(1.f, geom.height));

                    we::editor::viewportedit::ViewportInputEvent ve;
                    ve.x = localX;
                    ve.y = localY;
                    ve.deltaX = event.deltaX;
                    ve.deltaY = event.deltaY;
                    ve.scroll = event.wheelDeltaY;
                    ve.shift = event.shiftDown;
                    ve.ctrl = event.ctrlDown;
                    ve.alt = event.altDown;
                    switch (event.button) {
                    case we::runtime::kindui::MouseButton::Left:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::Left;
                        break;
                    case we::runtime::kindui::MouseButton::Right:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::Right;
                        break;
                    case we::runtime::kindui::MouseButton::Middle:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::Middle;
                        break;
                    default:
                        ve.button = we::editor::viewportedit::ViewportMouseButton::None;
                        break;
                    }

                    auto& interaction = m_ViewportEdit->Interaction();
                    switch (event.type) {
                    case we::runtime::kindui::MouseEventType::MouseDown:
                        return interaction.HandleMouseDown(ve);
                    case we::runtime::kindui::MouseEventType::MouseUp:
                        return interaction.HandleMouseUp(ve);
                    case we::runtime::kindui::MouseEventType::MouseMove:
                        return interaction.HandleMouseMove(ve);
                    case we::runtime::kindui::MouseEventType::MouseWheel:
                        return interaction.HandleScroll(ve);
                    }
                    return false;
                });
            }
        };
        shellDeps.onLayoutBuilt = [this](const ::we::editor::shell::DockLayoutBuildResult&) {};

        const auto shellResult = we::programs::editor::EditorShellBuilder::Build(*m_UIContext, shellDeps);
        m_OverlayHost = shellResult.overlayHost;
        m_TitleBar = shellResult.titleBar;
        m_StatusBar = shellResult.statusBar;

        m_WindowHitTestData.titleBar = m_TitleBar;
        we::platform::Platform::Get().SetWindowHitTest(
            m_Window,
            ::we::editor::mainframe::EditorWindowHitTest,
            &m_WindowHitTestData);

        SetRootWidget(shellResult.rootWidget);

        // Ensure Explorer TreeView is bound after panel construction.
        if (m_WorldOutliner) {
            if (auto tree = we::programs::editor::GetExplorerTreeView()) {
                m_WorldOutliner->Outliner().BindTreeView(tree);
                m_WorldOutliner->Outliner().RequestRebuild();
                m_WorldOutliner->Outliner().Tick(0.f);
            }
        }
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to build editor shell: " + std::string(e.what()));
        UnloadProjectWorkspace();
        OpenWeLauncher();
        m_Running = false;
        return;
    }

    UpdateWindowTitle();
    try {
        LogWidgetTreeLayout(m_RootWidget, "EditorShell");
    } catch (const std::exception& e) {
        HE_ERROR("[Startup] Failed to log widget tree: " + std::string(e.what()));
    }
}

void Editor::OpenProjectDialog() {
    we::platform::FileDialogDesc desc{};
    desc.mode = we::platform::FileDialogMode::OpenFile;
    desc.title = "Open Project";
    desc.filters = {
        { "WindEffects Project", "*.weproj" },
        { "All Files", "*.*" },
    };
    const auto files = we::platform::Platform::Get().ShowFileDialog(desc);
    if (!files.empty()) {
        EnterProjectWorkspace(files.front());
    }
}

} // namespace we::programs::editor