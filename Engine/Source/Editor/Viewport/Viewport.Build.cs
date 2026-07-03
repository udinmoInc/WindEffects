using IgniteBT.BuildSystem;

public class Viewport : ModuleRules
{
    public Viewport(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("RHI");
        Dependencies.Add("Renderer");
        Dependencies.Add("Engine");
        Dependencies.Add("Scene");
        Dependencies.Add("Application");

        Definitions.Add("VIEWPORT_EXPORTS");
    }
}
