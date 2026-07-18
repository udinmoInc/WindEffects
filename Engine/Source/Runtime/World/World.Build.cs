using IgniteBT.BuildSystem;

public class World : ModuleRules
{
    public World(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PrivateDependencies.Add("Renderer");

        AddOptionalThirdParty("glm");

        Definitions.Add("WORLD_EXPORTS");
    }
}
