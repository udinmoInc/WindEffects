using IgniteBT.BuildSystem;

public class RHI : ModuleRules
{
    public RHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");

        Definitions.Add("RHI_EXPORTS");
    }
}
