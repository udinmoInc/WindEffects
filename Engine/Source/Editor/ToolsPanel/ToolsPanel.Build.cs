using IgniteBT.BuildSystem;

public class ToolsPanel : ModuleRules
{
    public ToolsPanel(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Application");

        Definitions.Add("TOOLSPANEL_EXPORTS");
    }
}
