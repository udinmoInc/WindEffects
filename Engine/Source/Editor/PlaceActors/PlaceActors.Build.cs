using IgniteBT.BuildSystem;

public class PlaceActors : ModuleRules
{
    public PlaceActors(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("ContentBrowser");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Terrain");
        PublicDependencies.Add("TerrainEditor");

        Definitions.Add("PLACEACTORS_EXPORTS");
    }
}
