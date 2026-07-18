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
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("ContentBrowser");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Terrain");
        PrivateDependencies.Add("TerrainEditor");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("PLACEACTORS_EXPORTS");
    }
}
