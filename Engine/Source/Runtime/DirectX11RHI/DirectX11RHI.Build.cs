using IgniteBT.BuildSystem;

public class DirectX11RHI : ModuleRules
{
    public DirectX11RHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;
        BootstrapBinary();
        SetBinaryName("WEDX11RHI.dll");
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        Definitions.Add("DX11RHI_EXPORTS");
    }
}
