using IgniteBT.BuildSystem;

public class RHI : ModuleRules
{
    public RHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WERHI.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");

        // Public headers must stay free of Vulkan / DX / Metal / GL.
        Definitions.Add("RHI_EXPORTS");
    }
}
