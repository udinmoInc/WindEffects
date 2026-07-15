using IgniteBT.BuildSystem;

public class OpenGLESRHI : ModuleRules
{
    public OpenGLESRHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;
        BootstrapBinary();
        SetBinaryName("WEOpenGLESRHI.dll");
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        Definitions.Add("OPENGLESRHI_EXPORTS");
    }
}
