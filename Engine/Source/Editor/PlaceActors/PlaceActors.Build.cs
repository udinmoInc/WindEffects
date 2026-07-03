using IgniteBT.BuildSystem;

public class PlaceActors : ModuleRules
{
    public PlaceActors(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");
        Dependencies.Add("Scene");

        Definitions.Add("PLACEACTORS_EXPORTS");
    }
}
