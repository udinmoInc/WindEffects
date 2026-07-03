using IgniteBT.BuildSystem;

public class Renderer : ModuleRules
{
    public Renderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");

        // Add Vulkan SDK as optional dependency
        OptionalSDK("Vulkan");
        DefineIf(HasSDK("Vulkan"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("Vulkan"), "WE_HAS_VULKAN=0");

        Definitions.Add("RENDERER_EXPORTS");
    }
}
