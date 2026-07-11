using IgniteBT.BuildSystem;

public class Scene : ModuleRules
{
    public Scene(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add("Public/WindEffects/Runtime");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Renderer");

        AddOptionalThirdParty("glm");

        // Add Vulkan SDK as optional dependency
        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");

        Definitions.Add("SCENE_EXPORTS");
    }
}
