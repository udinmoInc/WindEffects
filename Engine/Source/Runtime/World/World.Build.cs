using IgniteBT.BuildSystem;

public class World : ModuleRules
{
    public World(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add(".");
        PrivateIncludePaths.Add("DefaultScene");
        PublicIncludePaths.Add("Environment");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("WORLD_EXPORTS");
    }
}
