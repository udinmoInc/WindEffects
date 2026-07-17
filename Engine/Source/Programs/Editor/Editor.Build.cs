using IgniteBT.BuildSystem;

public class Editor : ModuleRules
{
    public Editor(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WindeffectsEditor.exe");
        PublishAtConfigurationRoot();

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("MainFrame");
        PublicDependencies.Add("Viewport");
        PublicDependencies.Add("ContentBrowser");
        PublicDependencies.Add("WorldOutliner");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Menus");
        PublicDependencies.Add("ToolsPanel");
        PublicDependencies.Add("PlaceActors");
        PublicDependencies.Add("Environment");
        PublicDependencies.Add("EditorGridRenderer");
        PublicDependencies.Add("Terrain");
        PublicDependencies.Add("TerrainEditor");

        // RHI backends — load-order DLLs via ModuleBootstrap; ensure they are built/staged with Editor.
        // Unsupported backends (DX11/Metal/GL/GLES) register from WERHI.dll itself.
        PrivateDependencies.Add("NullRHI");
        PrivateDependencies.Add("VulkanRHI");
        PrivateDependencies.Add("DirectX12RHI");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
