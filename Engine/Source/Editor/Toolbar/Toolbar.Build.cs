using IgniteBT.BuildSystem;

public class Toolbar : ModuleRules
{
    public Toolbar(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("Menus");
        Dependencies.Add("Application");

        Definitions.Add("TOOLBAR_EXPORTS");
    }
}
