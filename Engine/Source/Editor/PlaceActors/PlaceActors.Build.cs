using IgniteBT.BuildSystem;

public class PlaceActors : ModuleRules
{
    public PlaceActors(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Application");

        Definitions.Add("PLACEACTORS_EXPORTS");
    }
}
