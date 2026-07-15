using IgniteBT.BuildSystem;

public class MetalRHI : ModuleRules
{
    public MetalRHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;
        BootstrapBinary();
        SetBinaryName("WEMetalRHI.dll");
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        Definitions.Add("METALRHI_EXPORTS");
    }
}
