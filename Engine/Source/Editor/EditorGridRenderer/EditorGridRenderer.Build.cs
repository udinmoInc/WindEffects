using IgniteBT.BuildSystem;

public class EditorGridRenderer : ModuleRules
{
    public EditorGridRenderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Application");

        Definitions.Add("EDITORGRIDRENDERER_EXPORTS");
    }
}
