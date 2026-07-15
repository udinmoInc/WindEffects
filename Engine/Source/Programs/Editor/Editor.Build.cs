using IgniteBT.BuildSystem;

public class Editor : ModuleRules
{
    public Editor(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WindeffectsEditor.exe");
        PublishAtConfigurationRoot();

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("MainFrame");
        PublicDependencies.Add("Viewport");
        PublicDependencies.Add("ContentBrowser");
        PublicDependencies.Add("WorldOutliner");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("Details");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Menus");
        PublicDependencies.Add("ToolsPanel");
        PublicDependencies.Add("PlaceActors");
        PublicDependencies.Add("Environment");
        PublicDependencies.Add("EditorGridRenderer");
        PublicDependencies.Add("Terrain");
        PublicDependencies.Add("TerrainEditor");

        OptionalSDK("VulkanSDK");
        OptionalSDK("SDL3");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
