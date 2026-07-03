using IgniteBT.BuildSystem;

public class MainFrame : ModuleRules
{
    public MainFrame(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Docking");
        PublicDependencies.Add("Application");

        // Add SDL3 as optional SDK
        OptionalSDK("SDL3");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        Definitions.Add("MAINFRAME_EXPORTS");
    }
}
