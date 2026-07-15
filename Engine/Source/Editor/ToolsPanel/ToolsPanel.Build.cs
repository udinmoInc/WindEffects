using IgniteBT.BuildSystem;

public class ToolsPanel : ModuleRules
{
    public ToolsPanel(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("Menus");
        PublicDependencies.Add("ContentBrowser");

        Definitions.Add("TOOLSPANEL_EXPORTS");
    }
}
