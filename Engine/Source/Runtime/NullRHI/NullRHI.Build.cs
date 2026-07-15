using IgniteBT.BuildSystem;

public class NullRHI : ModuleRules
{
    public NullRHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WENullRHI.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");

        Definitions.Add("NULLRHI_EXPORTS");
    }
}
