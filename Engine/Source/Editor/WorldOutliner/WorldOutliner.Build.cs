using IgniteBT.BuildSystem;

public class WorldOutliner : ModuleRules
{
    public WorldOutliner(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");
        Dependencies.Add("Scene");

        Definitions.Add("WORLDOUTLINER_EXPORTS");
    }
}
