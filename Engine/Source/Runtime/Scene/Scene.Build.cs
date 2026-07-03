using IgniteBT.BuildSystem;

public class Scene : ModuleRules
{
    public Scene(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");
        Dependencies.Add("Renderer");

        // Add GLM as optional third-party dependency
        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        // Add Vulkan SDK as optional dependency
        OptionalSDK("Vulkan");
        DefineIf(HasSDK("Vulkan"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("Vulkan"), "WE_HAS_VULKAN=0");

        Definitions.Add("SCENE_EXPORTS");
    }
}
