using IgniteBT.BuildSystem;

public class World : ModuleRules
{
    public World(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add("Public/WindEffects/Runtime");
        PublicIncludePaths.Add(".");
        PrivateIncludePaths.Add(".");
        PrivateIncludePaths.Add("DefaultScene");
        PrivateIncludePaths.Add("Environment");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");

        AddOptionalThirdParty("glm");

        Definitions.Add("WORLD_EXPORTS");
    }
}
