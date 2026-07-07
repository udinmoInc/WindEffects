using IgniteBT.BuildSystem;

public class Viewport : ModuleRules
{
    public Viewport(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Application");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("PlaceActors");

        OptionalSDK("VulkanSDK");
        OptionalSDK("SDL3");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        Definitions.Add("VIEWPORT_EXPORTS");
    }
}
