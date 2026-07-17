using IgniteBT.BuildSystem;

public class MainFrame : ModuleRules
{
    public MainFrame(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("UI");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("Toolbar");
        PublicDependencies.Add("Menus");

        Definitions.Add("MAINFRAME_EXPORTS");
    }
}
