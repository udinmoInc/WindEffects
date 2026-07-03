using IgniteBT.BuildSystem;

public class World : ModuleRules
{
    public World(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PrivateIncludePaths.Add("Environment");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");
        Dependencies.Add("Scene");

        Definitions.Add("WORLD_EXPORTS");
    }
}
