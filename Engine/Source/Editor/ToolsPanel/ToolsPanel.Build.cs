using IgniteBT.BuildSystem;

public class ToolsPanel : ModuleRules
{
    public ToolsPanel(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("Engine");
        Dependencies.Add("Application");

        Definitions.Add("TOOLSPANEL_EXPORTS");
    }
}
