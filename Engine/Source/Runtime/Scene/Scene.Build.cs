using IgniteBT.BuildSystem;

public class Scene : ModuleRules
{
    public Scene(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");

        Definitions.Add("SCENE_EXPORTS");
    }
}
