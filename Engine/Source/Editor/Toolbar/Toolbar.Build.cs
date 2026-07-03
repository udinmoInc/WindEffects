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

        Definitions.Add("TOOLBAR_EXPORTS");
    }
}
