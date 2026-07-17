using IgniteBT.BuildSystem;

public class EditorGridRenderer : ModuleRules
{
    public EditorGridRenderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("UI");
        PublicDependencies.Add("UIFramework");

        Definitions.Add("EDITORGRIDRENDERER_EXPORTS");
    }
}
