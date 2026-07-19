using IgniteBT.BuildSystem;

public class WorldOutliner : ModuleRules
{
    public WorldOutliner(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Presentation/interaction over World/Scene — never owns gameplay data.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("Undo");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("ViewportEdit");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("ContentBrowser");

        PrivateDependencies.Add("RHI");

        Definitions.Add("WORLDOUTLINER_EXPORTS");
    }
}
