using IgniteBT.BuildSystem;

public class Application : ModuleRules
{
    public Application(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("Engine");
        Dependencies.Add("Renderer");
        Dependencies.Add("Scene");
        Dependencies.Add("World");

        Definitions.Add("APPLICATION_EXPORTS");
    }
}
