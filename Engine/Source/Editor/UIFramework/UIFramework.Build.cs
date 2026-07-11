using IgniteBT.BuildSystem;

public class UIFramework : ModuleRules
{
    public UIFramework(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Application");
        PublicDependencies.Add("MainFrame");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Menus");
        PublicDependencies.Add("Viewport");
        PublicDependencies.Add("ContentBrowser");
        PublicDependencies.Add("WorldOutliner");
        PublicDependencies.Add("Details");
        PublicDependencies.Add("ToolsPanel");
        PublicDependencies.Add("Environment");
        PublicDependencies.Add("PlaceActors");

        OptionalSDK("VulkanSDK");
        OptionalSDK("SDL3");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        Definitions.Add("UIFRAMEWORK_EXPORTS");
    }
}
