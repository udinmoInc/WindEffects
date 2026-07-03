using IgniteBT.BuildSystem;

public class Renderer : ModuleRules
{
    public Renderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");
        Dependencies.Add("RHI");
        Dependencies.Add("Engine");

        Definitions.Add("RENDERER_EXPORTS");
    }
}
