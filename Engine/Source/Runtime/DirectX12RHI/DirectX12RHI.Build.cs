using IgniteBT.BuildSystem;

public class DirectX12RHI : ModuleRules
{
    public DirectX12RHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;
        BootstrapBinary();
        SetBinaryName("WEDX12RHI.dll");
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        Definitions.Add("DX12RHI_EXPORTS");
    }
}
