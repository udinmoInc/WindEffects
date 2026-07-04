using IgniteBT.BuildSystem;

public class Scene : ModuleRules
{
    public Scene(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Renderer");

        // Add GLM as optional third-party dependency
        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        // Add Vulkan SDK as optional dependency
        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");

        Definitions.Add("SCENE_EXPORTS");
    }
}
