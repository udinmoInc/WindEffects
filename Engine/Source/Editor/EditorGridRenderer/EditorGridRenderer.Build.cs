using IgniteBT.BuildSystem;

public class EditorGridRenderer : ModuleRules
{
    public EditorGridRenderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("UIFramework");

        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");

        Definitions.Add("EDITORGRIDRENDERER_EXPORTS");
    }
}
