using IgniteBT.BuildSystem;

public class MainFrame : ModuleRules
{
    public MainFrame(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("Docking");

        Definitions.Add("MAINFRAME_EXPORTS");
    }
}
