using IgniteBT.BuildSystem;

public class EditorGridRenderer : ModuleRules
{
    public EditorGridRenderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("RHI");
        Dependencies.Add("Renderer");
        Dependencies.Add("Application");

        Definitions.Add("EDITORGRIDRENDERER_EXPORTS");
    }
}
