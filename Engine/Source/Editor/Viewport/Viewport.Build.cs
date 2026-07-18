using IgniteBT.BuildSystem;

public class Viewport : ModuleRules
{
    public Viewport(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");
        PrivateDependencies.Add("Toolbar");
        PrivateDependencies.Add("PlaceActors");

        Definitions.Add("VIEWPORT_EXPORTS");
    }
}
