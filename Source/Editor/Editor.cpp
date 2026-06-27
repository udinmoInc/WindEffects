#include "Editor.hpp"
#include "../Core/Logger.hpp"

// HouseUI Layout and Widget Headers
#include "HouseUI/Layout/Box.hpp"
#include "HouseUI/Layout/Splitter.hpp"
#include "HouseUI/Layout/ScrollLayout.hpp"
#include "HouseUI/Widgets/Button.hpp"
#include "HouseUI/Widgets/Label.hpp"
#include "HouseUI/Widgets/TextBox.hpp"
#include "HouseUI/Widgets/ViewportWidget.hpp"
#include "HouseUI/Widgets/Panel.hpp"
#include "HouseUI/Widgets/TreeView.hpp"
#include "HouseUI/Widgets/PropertyEditor.hpp"
#include "HouseUI/Widgets/MenuBar.hpp"
#include "HouseUI/Widgets/Toolbar.hpp"
#include "HouseUI/Widgets/TabWidget.hpp"
#include "HouseUI/Widgets/ViewportOverlay.hpp"
#include "HouseUI/Widgets/ContentBrowser.hpp"
#include "HouseUI/Widgets/StatusBar.hpp"
#include "HouseUI/Widgets/SearchBox.hpp"
#include "HouseUI/Core/Icon.hpp"

#include <iostream>
#include <stdexcept>
#include <chrono>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvgrast.h>

#include "../HouseUI/Widgets/LogoSVG.hpp"

static VkDescriptorSet LoadSVGSingle(std::shared_ptr<HouseEngine::VulkanContext>& context, HouseEngine::UI::UIRenderer* uiRenderer, int width, int height) {
    std::string svgStr = g_WindEffectsLogoSVG;
    std::vector<char> svgCopy(svgStr.begin(), svgStr.end());
    svgCopy.push_back('\0');

    NSVGimage* image = nsvgParse(svgCopy.data(), "px", 96);
    if (!image) {
        HE_ERROR("Failed to parse embedded SVG");
        return VK_NULL_HANDLE;
    }

    // Force all shapes to be white
    for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {
        shape->fill.type = NSVG_PAINT_COLOR; // 1 = NSVG_PAINT_COLOR
        shape->fill.color = 0xFFFFFFFF; // White with full alpha
    }
    
    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(image);
        return VK_NULL_HANDLE;
    }

    std::vector<uint8_t> rgbaBuffer(width * height * 4, 0);
    float scale = (float)width / image->width;
    float scaleY = (float)height / image->height;
    if (scaleY < scale) scale = scaleY; // fit
    
    nsvgRasterize(rast, image, 0, 0, scale, rgbaBuffer.data(), width, height, width * 4);
    
    nsvgDeleteRasterizer(rast);
    nsvgDelete(image);

    // Vulkan upload
    VkDevice device = context->GetDevice();
    VkDeviceSize imageSize = width * height * 4;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    context->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, rgbaBuffer.data(), (size_t)imageSize);
    vkUnmapMemory(device, stagingBufferMemory);

    VkImage vkImage;
    VkDeviceMemory vkMemory;
    context->CreateImage(width, height, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vkImage, vkMemory);

    context->TransitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkCommandBuffer cmd = context->BeginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = { (uint32_t)width, (uint32_t)height, 1 };
    vkCmdCopyBufferToImage(cmd, stagingBuffer, vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    context->EndSingleTimeCommands(cmd);

    context->TransitionImageLayout(vkImage, VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    VkImageView view = context->CreateImageView(vkImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    
    VkSampler sampler;
    vkCreateSampler(device, &samplerInfo, nullptr, &sampler);

    return uiRenderer->RegisterTexture(view, sampler);
}

namespace HouseEngine {

Editor::Editor() {
    HE_INFO("Initializing SDL3 platform...");
    InitSDL();
    HE_INFO("SDL3 platform successfully initialized.");

    HE_INFO("Creating Vulkan Context...");
    m_Context = std::make_shared<VulkanContext>(m_Window);
    HE_INFO("Vulkan Context created successfully.");

    HE_INFO("Initializing Vulkan Renderer & Swapchain...");
    m_Renderer = std::make_shared<Renderer>(m_Context, m_Window);
    m_RenderGraph = std::make_shared<RenderGraph>(m_Renderer);
    HE_INFO("Vulkan Renderer & Swapchain ready.");

    HE_INFO("Compiling & loading Scene render pipelines...");
    m_SceneRenderer = std::make_shared<SceneRenderer>(m_Context, m_Renderer->GetOffscreenRenderPass(), m_Renderer->GetCameraDescLayout());
    m_GridRenderer = std::make_shared<GridRenderer>(m_Context, m_Renderer->GetOffscreenRenderPass(), m_Renderer->GetCameraDescLayout());
    HE_INFO("Pipelines loaded successfully.");

    HE_INFO("Initializing Editor Scene Graph...");
    m_Camera = std::make_shared<EditorCamera>();
    m_Scene = std::make_shared<Scene>(m_Context, m_SceneRenderer);
    m_Scene->InitializeDefaultScene(m_Renderer->GetCameraBuffer());
    HE_INFO("Default Scene populated.");

    HE_INFO("Initializing HouseUI Rendering Subsystem...");
    m_UIRenderer = std::make_unique<UI::UIRenderer>();
    if (!m_UIRenderer->Init(m_Context, m_Renderer->GetSwapchainRenderPass())) {
        throw std::runtime_error("Failed to initialize HouseUI Renderer!");
    }
    m_UIEventSystem = std::make_shared<UI::EventSystem>();

    HE_INFO("Building HouseUI Editor Workspace Panel Tree...");
    BuildEditorUI();
    m_UIEventSystem->SetRootWidget(m_RootWidget);
    
    // Initialize icon registry
    UI::IconRegistry::Get().InitializeDefaultIcons();

    HE_INFO("WindEffects Engine Editor successfully bootstrapped using HouseUI.");
}

Editor::~Editor() {
    Shutdown();
}

void Editor::InitSDL() {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        throw std::runtime_error("Failed to initialize SDL3!");
    }

    m_Window = SDL_CreateWindow(
        "WindEffects Engine - Editor Viewport (HouseUI v0.1)",
        1280, 720,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_BORDERLESS
    );

    if (!m_Window) {
        throw std::runtime_error("Failed to create SDL3 window!");
    }

    SDL_SetWindowHitTest(m_Window, HitTestCallback, this);
}

void Editor::BuildEditorUI() {
    // 1. Root Vertical Box
    auto rootVBox = std::make_shared<UI::VerticalBox>();
    rootVBox->SetSpacing(0.0f);

    // 2. Menu Bar
    auto menuBar = std::make_shared<UI::MenuBar>();
    
    // File menu
    std::vector<std::shared_ptr<UI::MenuItem>> fileItems;
    auto newItem = std::make_shared<UI::MenuItem>();
    newItem->label = "New";
    newItem->shortcut = "Ctrl+N";
    newItem->onClick = []() { /* TODO: New scene */ };
    fileItems.push_back(newItem);
    
    auto openItem = std::make_shared<UI::MenuItem>();
    openItem->label = "Open";
    openItem->shortcut = "Ctrl+O";
    openItem->onClick = []() { /* TODO: Open scene */ };
    fileItems.push_back(openItem);
    
    auto saveItem = std::make_shared<UI::MenuItem>();
    saveItem->label = "Save";
    saveItem->shortcut = "Ctrl+S";
    saveItem->onClick = []() { /* TODO: Save scene */ };
    fileItems.push_back(saveItem);
    
    auto exitItem = std::make_shared<UI::MenuItem>();
    exitItem->label = "Exit";
    exitItem->shortcut = "Alt+F4";
    exitItem->onClick = [this]() { m_Running = false; };
    fileItems.push_back(exitItem);
    
    menuBar->AddMenu("File", fileItems);
    
    // Edit menu
    std::vector<std::shared_ptr<UI::MenuItem>> editItems;
    auto undoItem = std::make_shared<UI::MenuItem>();
    undoItem->label = "Undo";
    undoItem->shortcut = "Ctrl+Z";
    undoItem->onClick = []() { /* TODO: Undo */ };
    editItems.push_back(undoItem);
    
    auto redoItem = std::make_shared<UI::MenuItem>();
    redoItem->label = "Redo";
    redoItem->shortcut = "Ctrl+Y";
    redoItem->onClick = []() { /* TODO: Redo */ };
    editItems.push_back(redoItem);
    
    menuBar->AddMenu("Edit", editItems);
    
    // View menu
    std::vector<std::shared_ptr<UI::MenuItem>> viewItems;
    auto resetCamItem = std::make_shared<UI::MenuItem>();
    resetCamItem->label = "Reset Camera";
    resetCamItem->onClick = [this]() { m_Camera->Reset(); };
    viewItems.push_back(resetCamItem);
    
    menuBar->AddMenu("View", viewItems);
    
    // Assets menu
    std::vector<std::shared_ptr<UI::MenuItem>> assetsItems;
    menuBar->AddMenu("Assets", assetsItems);

    // Build menu
    std::vector<std::shared_ptr<UI::MenuItem>> buildItems;
    menuBar->AddMenu("Build", buildItems);

    // Tools menu
    std::vector<std::shared_ptr<UI::MenuItem>> toolsItems;
    menuBar->AddMenu("Tools", toolsItems);

    // Window menu
    std::vector<std::shared_ptr<UI::MenuItem>> windowItems;
    menuBar->AddMenu("Window", windowItems);

    // Help menu
    std::vector<std::shared_ptr<UI::MenuItem>> helpItems;
    menuBar->AddMenu("Help", helpItems);

    // 3. Main Toolbar
    auto toolbar = std::make_shared<UI::Toolbar>();
    toolbar->SetHeight(48.0f); // 48px height
    
    // File
    toolbar->AddTool(UI::Icons::DocumentName, "New", []() { /* TODO */ }, "New Scene");
    toolbar->AddTool(UI::Icons::FolderName, "Open", []() { /* TODO */ }, "Open Scene");
    toolbar->AddTool(UI::Icons::SaveName, "Save", []() { /* TODO */ }, "Save Scene");
    toolbar->AddSeparator();
    
    // History
    toolbar->AddTool(UI::Icons::UndoName, "Undo", []() { /* TODO */ }, "Undo");
    toolbar->AddTool(UI::Icons::RedoName, "Redo", []() { /* TODO */ }, "Redo");
    toolbar->AddSeparator();

    // Compile
    toolbar->AddTool(UI::Icons::CodeName, "Compile", []() { /* TODO */ }, "Compile Code");
    toolbar->AddTool(UI::Icons::BuildName, "Build", []() { /* TODO */ }, "Build Project");
    toolbar->AddSeparator();
    
    // Playback
    toolbar->AddTool(UI::Icons::PlayName, "Play", []() { /* TODO */ }, "Play", true); // isPlayButton = true
    toolbar->AddTool(UI::Icons::PauseName, "Pause", []() { /* TODO */ }, "Pause");
    toolbar->AddTool(UI::Icons::StopName, "Stop", []() { /* TODO */ }, "Stop");
    toolbar->AddSeparator();

    // Package
    toolbar->AddTool(UI::Icons::PackageName, "Package", []() { /* TODO */ }, "Package Project");
    toolbar->AddTool(UI::Icons::LayersName, "Launch", []() { /* TODO */ }, "Launch Project");
    
    // Load SVG Logo
    VkDescriptorSet logoDesc = LoadSVGSingle(m_Context, m_UIRenderer.get(), 20, 20);

    // Create custom TitleBar
    m_TitleBar = std::make_shared<UI::TitleBar>(m_Window, "WindEffects Engine", logoDesc, menuBar);
    m_TitleBar->Construct();
    rootVBox->AddChild(m_TitleBar);
    
    // Add Row 2 (Main Toolbar)
    toolbar->SetHeight(36.0f); // Make it 36px high as requested
    rootVBox->AddChild(toolbar);

    // 4. Main Splitter Panel (Left sidebar vs Right workspace)
    auto mainSplitter = std::make_shared<UI::Splitter>(UI::Orientation::Horizontal, 0.20f);
    
    // Left Sidebar: Outliner Panel
    auto outlinerPanel = std::make_shared<UI::Panel>("World Outliner");
    
    auto treeView = std::make_shared<UI::TreeView>();
    treeView->SetOnSelectionChanged([this](const std::string& id) {
        // Convert string ID to index for now
        try {
            int idx = std::stoi(id);
            m_Scene->SetSelectedEntityIndex(idx);
            UpdateInspectorPanel();
        } catch (...) {}
    });
    m_OutlinerTreeView = treeView;
    outlinerPanel->SetContent(treeView);

    mainSplitter->SetFirstChild(outlinerPanel);

    // Right Workspace (Center splits vs Right details)
    auto workspaceRightSplitter = std::make_shared<UI::Splitter>(UI::Orientation::Horizontal, 0.75f);

    // Center layout: Viewport (Top) vs Bottom panels
    auto centerSplitter = std::make_shared<UI::Splitter>(UI::Orientation::Vertical, 0.70f);

    // Main Tab Area
    auto mainTabWidget = std::make_shared<UI::TabWidget>();
    mainTabWidget->SetTabHeight(32.0f);
    
    mainTabWidget->AddTab("Viewport");
    
    auto viewportContainer = std::make_shared<UI::VerticalBox>();
    viewportContainer->SetSpacing(0.0f);
    
    m_ViewportWidget = std::make_shared<UI::ViewportWidget>(m_Renderer, m_Camera, m_Scene, m_UIRenderer.get());
    
    // Create Embedded Viewport Toolbar
    auto viewportToolbar = std::make_shared<UI::Toolbar>();
    viewportToolbar->SetHeight(32.0f); // 32px compact height
    viewportToolbar->SetFloating(false);
    
    viewportToolbar->AddTool(UI::Icons::EyeName, "", []() { /* TODO */ }, "Move");
    viewportToolbar->AddTool(UI::Icons::LayersName, "", []() { /* TODO */ }, "Rotate");
    viewportToolbar->AddTool(UI::Icons::CubeName, "", []() { /* TODO */ }, "Scale");
    viewportToolbar->AddSeparator();
    viewportToolbar->AddTool(UI::Icons::PlaneName, "", []() { /* TODO */ }, "Local");
    viewportToolbar->AddTool(UI::Icons::InfoName, "", []() { /* TODO */ }, "World");
    viewportToolbar->AddTool(UI::Icons::SnapName, "", []() { /* TODO */ }, "Snap");
    viewportToolbar->AddSeparator();
    viewportToolbar->AddTool(UI::Icons::PerspectiveName, "", []() { /* TODO */ }, "Perspective");
    viewportToolbar->AddTool(UI::Icons::LitName, "", []() { /* TODO */ }, "Lit");
    viewportToolbar->AddTool(UI::Icons::CameraName, "", []() { /* TODO */ }, "Camera");
    viewportToolbar->AddSeparator();
    viewportToolbar->AddTool(UI::Icons::GridName, "", []() { /* TODO */ }, "Grid");
    viewportToolbar->AddTool(UI::Icons::SettingsName, "", []() { /* TODO */ }, "Statistics");
    
    viewportContainer->AddChild(viewportToolbar);
    viewportContainer->AddChild(m_ViewportWidget);
    
    mainTabWidget->SetTabContent(0, viewportContainer);
    
    // Add stub tabs
    mainTabWidget->AddTab("World");
    mainTabWidget->SetTabContent(1, std::make_shared<UI::Panel>("World Settings"));
    
    mainTabWidget->AddTab("Blueprints");
    mainTabWidget->SetTabContent(2, std::make_shared<UI::Panel>("Blueprints"));
    
    mainTabWidget->AddTab("Animation");
    mainTabWidget->SetTabContent(3, std::make_shared<UI::Panel>("Animation Graph"));

    mainTabWidget->AddTab("Material");
    mainTabWidget->SetTabContent(4, std::make_shared<UI::Panel>("Material Editor"));

    centerSplitter->SetFirstChild(mainTabWidget);
    
    // Bottom area: Content Browser and Console (Tabbed)
    auto bottomTabWidget = std::make_shared<UI::TabWidget>();
    bottomTabWidget->SetTabHeight(32.0f);
    
    // Content Browser tab
    bottomTabWidget->AddTab("Content Browser");
    auto contentBrowser = std::make_shared<UI::ContentBrowser>();
    
    // Add some sample content (Folders)
    UI::ContentItem folder1{"0", "Maps", "folder", UI::Icons::FolderName, true, false};
    UI::ContentItem folder2{"1", "Materials", "folder", UI::Icons::FolderName, true, false};
    UI::ContentItem folder3{"2", "Meshes", "folder", UI::Icons::FolderName, true, false};
    UI::ContentItem folder4{"3", "Textures", "folder", UI::Icons::FolderName, true, false};
    contentBrowser->AddItem(folder1);
    contentBrowser->AddItem(folder2);
    contentBrowser->AddItem(folder3);
    contentBrowser->AddItem(folder4);
    
    // Add sample content (Assets)
    UI::ContentItem asset1{"10", "PlayerCharacter", "Blueprint", UI::Icons::DocumentName, false, true};
    UI::ContentItem asset2{"11", "M_Concrete", "Material", UI::Icons::MaterialName, false, false};
    UI::ContentItem asset3{"12", "SM_Crate", "StaticMesh", UI::Icons::CubeName, false, false};
    UI::ContentItem asset4{"13", "T_Brick_D", "Texture2D", UI::Icons::TextureName, false, false};
    UI::ContentItem asset5{"14", "Level_01", "Map", UI::Icons::LayersName, false, true};
    contentBrowser->AddItem(asset1);
    contentBrowser->AddItem(asset2);
    contentBrowser->AddItem(asset3);
    contentBrowser->AddItem(asset4);
    contentBrowser->AddItem(asset5);
    
    bottomTabWidget->SetTabContent(0, contentBrowser);
    
    // Output Log tab
    bottomTabWidget->AddTab("Output Log");
    auto logPanel = std::make_shared<UI::Panel>("Output Log");
    logPanel->SetCollapsible(false);
    m_ConsoleList = std::make_shared<UI::VerticalBox>();
    m_ConsoleList->SetSpacing(2.0f);
    logPanel->SetContent(m_ConsoleList);
    bottomTabWidget->SetTabContent(1, logPanel);
    
    // Console tab
    bottomTabWidget->AddTab("Console");
    auto consolePanel = std::make_shared<UI::Panel>("Console");
    consolePanel->SetCollapsible(false);
    bottomTabWidget->SetTabContent(2, consolePanel);

    // Profiler tab
    bottomTabWidget->AddTab("Profiler");
    auto profilerPanel = std::make_shared<UI::Panel>("Profiler Data");
    profilerPanel->SetCollapsible(false);
    bottomTabWidget->SetTabContent(3, profilerPanel);
    
    centerSplitter->SetSecondChild(bottomTabWidget);
    workspaceRightSplitter->SetFirstChild(centerSplitter);

    // Right Sidebar: Details/Inspector Panel
    auto inspectorPanel = std::make_shared<UI::Panel>("Details");
    auto propertyEditor = std::make_shared<UI::PropertyEditor>();
    m_PropertyEditor = propertyEditor;
    inspectorPanel->SetContent(propertyEditor);

    workspaceRightSplitter->SetSecondChild(inspectorPanel);
    mainSplitter->SetSecondChild(workspaceRightSplitter);

    rootVBox->AddChild(mainSplitter);
    
    // 5. Status Bar
    auto statusBar = std::make_shared<UI::StatusBar>();
    statusBar->Construct();
    m_StatusBar = statusBar;
    rootVBox->AddChild(statusBar);
    m_RootWidget = rootVBox;

    // Trigger initial panel populate
    UpdateOutlinerPanel();
    UpdateInspectorPanel();
}

void Editor::UpdateOutlinerPanel() {
    auto treeView = std::static_pointer_cast<UI::TreeView>(m_OutlinerTreeView);
    if (!treeView) return;
    
    treeView->Clear();
    
    auto& entities = m_Scene->GetEntities();
    int selectedIdx = m_Scene->GetSelectedEntityIndex();

    for (size_t i = 0; i < entities.size(); ++i) {
        auto node = std::make_shared<UI::TreeNode>();
        node->id = std::to_string(i);
        node->label = entities[i].Name;
        
        // Set icon based on entity type
        if (entities[i].Type == EntityType::Plane) {
            node->iconName = UI::Icons::PlaneName;
        } else if (entities[i].Type == EntityType::DirectionalLight) {
            node->iconName = UI::Icons::LightName;
        } else if (entities[i].Type == EntityType::CameraIcon) {
            node->iconName = UI::Icons::CameraName;
        } else {
            node->iconName = UI::Icons::CubeName;
        }
        
        m_OutlinerTreeView->AddItem(node);
    }
    
    // Set selected item
    if (selectedIdx >= 0) {
        m_OutlinerTreeView->SetSelectedId(std::to_string(selectedIdx));
    }
    
    m_LastEntityCount = entities.size();
    m_LastSelectedEntity = selectedIdx;
}

void Editor::UpdateInspectorPanel() {
    auto propertyEditor = std::static_pointer_cast<UI::PropertyEditor>(m_PropertyEditor);
    if (!propertyEditor) return;
    
    propertyEditor->Clear();

    int selectedIdx = m_Scene->GetSelectedEntityIndex();
    if (selectedIdx < 0 || selectedIdx >= static_cast<int>(m_Scene->GetEntities().size())) {
        return;
    }

    auto& entity = m_Scene->GetEntities()[selectedIdx];

    // Transform category
    UI::Property nameProp;
    nameProp.name = "Name";
    nameProp.category = "Transform";
    nameProp.type = UI::PropertyType::String;
    nameProp.value = entity.Name;
    nameProp.defaultValue = entity.Name;
    nameProp.onValueChanged = [&entity](const std::string& val) { entity.Name = val; };
    propertyEditor->AddProperty(nameProp);
    
    UI::Property posXProp;
    posXProp.name = "Position X";
    posXProp.category = "Transform";
    posXProp.type = UI::PropertyType::Float;
    posXProp.value = std::to_string(entity.Position.x);
    posXProp.defaultValue = "0.0";
    posXProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Position.x = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(posXProp);
    
    UI::Property posYProp;
    posYProp.name = "Position Y";
    posYProp.category = "Transform";
    posYProp.type = UI::PropertyType::Float;
    posYProp.value = std::to_string(entity.Position.y);
    posYProp.defaultValue = "0.0";
    posYProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Position.y = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(posYProp);
    
    UI::Property posZProp;
    posZProp.name = "Position Z";
    posZProp.category = "Transform";
    posZProp.type = UI::PropertyType::Float;
    posZProp.value = std::to_string(entity.Position.z);
    posZProp.defaultValue = "0.0";
    posZProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Position.z = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(posZProp);
    
    // Rotation category
    UI::Property rotXProp;
    rotXProp.name = "Rotation X";
    rotXProp.category = "Rotation";
    rotXProp.type = UI::PropertyType::Float;
    rotXProp.value = std::to_string(entity.Rotation.x);
    rotXProp.defaultValue = "0.0";
    rotXProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Rotation.x = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(rotXProp);
    
    UI::Property rotYProp;
    rotYProp.name = "Rotation Y";
    rotYProp.category = "Rotation";
    rotYProp.type = UI::PropertyType::Float;
    rotYProp.value = std::to_string(entity.Rotation.y);
    rotYProp.defaultValue = "0.0";
    rotYProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Rotation.y = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(rotYProp);
    
    UI::Property rotZProp;
    rotZProp.name = "Rotation Z";
    rotZProp.category = "Rotation";
    rotZProp.type = UI::PropertyType::Float;
    rotZProp.value = std::to_string(entity.Rotation.z);
    rotZProp.defaultValue = "0.0";
    rotZProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Rotation.z = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(rotZProp);
    
    // Scale category
    UI::Property scaleXProp;
    scaleXProp.name = "Scale X";
    scaleXProp.category = "Scale";
    scaleXProp.type = UI::PropertyType::Float;
    scaleXProp.value = std::to_string(entity.Scale.x);
    scaleXProp.defaultValue = "1.0";
    scaleXProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Scale.x = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(scaleXProp);
    
    UI::Property scaleYProp;
    scaleYProp.name = "Scale Y";
    scaleYProp.category = "Scale";
    scaleYProp.type = UI::PropertyType::Float;
    scaleYProp.value = std::to_string(entity.Scale.y);
    scaleYProp.defaultValue = "1.0";
    scaleYProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Scale.y = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(scaleYProp);
    
    UI::Property scaleZProp;
    scaleZProp.name = "Scale Z";
    scaleZProp.category = "Scale";
    scaleZProp.type = UI::PropertyType::Float;
    scaleZProp.value = std::to_string(entity.Scale.z);
    scaleZProp.defaultValue = "1.0";
    scaleZProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Scale.z = std::stof(val); } catch (...) {}
    };
    propertyEditor->AddProperty(scaleZProp);
    
    // Rendering category
    UI::Property modeProp;
    modeProp.name = "Shading Mode";
    modeProp.category = "Rendering";
    modeProp.type = UI::PropertyType::Enum;
    modeProp.value = std::to_string(entity.Mode);
    modeProp.defaultValue = "0";
    modeProp.enumOptions = {"Lit", "Unlit", "Wireframe"};
    modeProp.onValueChanged = [&entity](const std::string& val) {
        try { entity.Mode = std::stoi(val); } catch (...) {}
    };
    propertyEditor->AddProperty(modeProp);
}

void Editor::UpdateConsolePanel() {
    auto logs = Logger::GetNewLogs();
    if (logs.empty()) return;

    for (const auto& log : logs) {
        UI::Color col = UI::Color{ 0.85f, 0.85f, 0.88f, 1.0f }; // Info
        if (log.level == Logger::Level::Warning) {
            col = UI::Color{ 0.95f, 0.8f, 0.25f, 1.0f }; // Warning yellow
        } else if (log.level == Logger::Level::Error) {
            col = UI::Color{ 0.9f, 0.15f, 0.15f, 1.0f }; // Error red
        } else if (log.level == Logger::Level::Debug) {
            col = UI::Color{ 0.3f, 0.6f, 0.9f, 1.0f }; // Debug blue
        }

        auto lbl = std::make_shared<UI::Label>(log.formattedText, col, 12.0f);
        m_ConsoleList->AddChild(lbl);
    }

    // Limit buffer length
    auto& children = m_ConsoleList->GetChildren();
    if (children.size() > 100) {
        m_ConsoleList->ClearChildren();
    }
}

void Editor::Run() {
    MainLoop();
}

void Editor::MainLoop() {
    uint64_t lastTime = SDL_GetPerformanceCounter();
    double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    while (m_Running) {
        // 1. Process Platform Window Events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                m_Running = false;
            }
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(m_Window)) {
                m_Running = false;
            }

            // Route events to HouseUI EventSystem
            UI::MouseEvent mouseEvent{};
            mouseEvent.altDown = (SDL_GetModState() & SDL_KMOD_ALT) != 0;
            mouseEvent.shiftDown = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
            mouseEvent.ctrlDown = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;

            bool isMouse = false;

            if (event.type == SDL_EVENT_MOUSE_MOTION) {
                mouseEvent.type = UI::MouseEventType::MouseMove;
                mouseEvent.position = UI::Point{ event.motion.x, event.motion.y };
                isMouse = true;
            } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                mouseEvent.type = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ? UI::MouseEventType::MouseDown : UI::MouseEventType::MouseUp;
                mouseEvent.position = UI::Point{ event.button.x, event.button.y };
                if (event.button.button == SDL_BUTTON_LEFT) mouseEvent.button = UI::MouseButton::Left;
                else if (event.button.button == SDL_BUTTON_RIGHT) mouseEvent.button = UI::MouseButton::Right;
                else if (event.button.button == SDL_BUTTON_MIDDLE) mouseEvent.button = UI::MouseButton::Middle;
                isMouse = true;
            } else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                // Get mouse coordinates for wheel focus routing
                float mx, my;
                SDL_GetMouseState(&mx, &my);
                mouseEvent.type = UI::MouseEventType::MouseWheel;
                mouseEvent.position = UI::Point{ mx, my };
                mouseEvent.wheelDeltaX = event.wheel.x;
                mouseEvent.wheelDeltaY = event.wheel.y;
                isMouse = true;
            }

            if (isMouse) {
                m_UIEventSystem->ProcessMouseEvent(mouseEvent);
            }

            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                UI::KeyEvent keyEvent{};
                keyEvent.type = (event.type == SDL_EVENT_KEY_DOWN) ? UI::KeyEventType::KeyDown : UI::KeyEventType::KeyUp;
                keyEvent.keycode = event.key.key;
                keyEvent.altDown = (SDL_GetModState() & SDL_KMOD_ALT) != 0;
                keyEvent.shiftDown = (SDL_GetModState() & SDL_KMOD_SHIFT) != 0;
                keyEvent.ctrlDown = (SDL_GetModState() & SDL_KMOD_CTRL) != 0;
                m_UIEventSystem->ProcessKeyEvent(keyEvent);
            }
        }

        if (!m_Running) {
            break;
        }

        // Check minimization
        SDL_WindowFlags windowFlags = SDL_GetWindowFlags(m_Window);
        if (windowFlags & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(16);
            continue;
        }

        // 2. Frame timers
        uint64_t now = SDL_GetPerformanceCounter();
        float dt = static_cast<float>((now - lastTime) / frequency);
        lastTime = now;

        // Clamp delta time to avoid huge spikes
        if (dt > 0.1f) dt = 0.1f;

        // 3. Update active panels & widgets
        if (m_Scene->GetEntities().size() != m_LastEntityCount || m_Scene->GetSelectedEntityIndex() != m_LastSelectedEntity) {
            UpdateOutlinerPanel();
            UpdateInspectorPanel();
        }
        UpdateConsolePanel();

        // 4. Update cameras & ticks
        m_Camera->Update(dt);
        m_Scene->Update();

        // Tick retained widgets
        m_RootWidget->Tick(dt);

        // 5. Update Uniform buffers
        Renderer::CameraUniform cameraUBO{};
        cameraUBO.view = m_Camera->GetViewMatrix();
        cameraUBO.proj = m_Camera->GetProjectionMatrix();
        cameraUBO.pos = m_Camera->GetPosition();
        cameraUBO.padding = 0.0f;
        m_Renderer->UpdateCameraBuffer(cameraUBO);

        // 6. Draw Frame
        // Flush any deferred viewport resize BEFORE acquiring a swapchain image
        if (m_ViewportWidget) m_ViewportWidget->FlushPendingResize();

        if (m_Renderer->BeginFrame()) {
            VkCommandBuffer cmd = m_Renderer->GetCommandBuffer();

            // Pass 1: Offscreen Viewport Scene Pass
            m_RenderGraph->BeginOffscreenPass(cmd);
            m_SceneRenderer->DrawSkybox(cmd, m_Renderer->GetCameraDescSet());
            m_GridRenderer->Draw(cmd, m_Renderer->GetCameraDescSet());

            for (const auto& entity : m_Scene->GetEntities()) {
                std::string meshName = "Cube";
                if (entity.Type == EntityType::Plane) {
                    meshName = "Plane";
                }
                m_SceneRenderer->DrawMesh(cmd, meshName, entity.DescriptorSet, entity.Mode);
            }
            m_RenderGraph->EndOffscreenPass(cmd);

            // Pass 2: UI Swapchain Pass
            m_RenderGraph->BeginSwapchainPass(cmd);
            
            // Draw HouseUI widget tree
            m_UIRenderer->Render(
                cmd,
                m_Renderer->GetSwapchainWidth(),
                m_Renderer->GetSwapchainHeight(),
                m_RootWidget
            );

            m_RenderGraph->EndSwapchainPass(cmd);

            // Present Swapchain Image
            m_Renderer->EndFrame();
        }
    }
}

void Editor::Shutdown() {
    vkDeviceWaitIdle(m_Context->GetDevice());

    // Destroy HouseUI
    m_ViewportWidget.reset();
    m_RootWidget.reset();
    
    if (m_UIRenderer) {
        m_UIRenderer->Shutdown();
        m_UIRenderer.reset();
    }
    m_UIEventSystem.reset();

    // Destroy Scene renderers
    m_Scene.reset();
    m_Camera.reset();
    m_GridRenderer.reset();
    m_SceneRenderer.reset();
    m_RenderGraph.reset();
    m_Renderer.reset();
    m_Context.reset();

    // Destroy SDL3
    if (m_Window) {
        SDL_DestroyWindow(m_Window);
        m_Window = nullptr;
    }
    SDL_Quit();
}

SDL_HitTestResult SDLCALL Editor::HitTestCallback(SDL_Window* win, const SDL_Point* area, void* data) {
    (void)win;
    Editor* editor = static_cast<Editor*>(data);
    if (editor && editor->m_TitleBar) {
        return editor->m_TitleBar->HitTest(*area);
    }
    return SDL_HITTEST_NORMAL;
}

} // namespace HouseEngine
