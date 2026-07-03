using IgniteBT.BuildSystem;

public class Docking : ModuleRules
{
    public Docking(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");

        Definitions.Add("DOCKING_EXPORTS");
    }
}
