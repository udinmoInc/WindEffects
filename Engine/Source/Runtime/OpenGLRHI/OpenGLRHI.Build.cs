using IgniteBT.BuildSystem;

public class OpenGLRHI : ModuleRules
{
    public OpenGLRHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;
        BootstrapBinary();
        SetBinaryName("WEOpenGLRHI.dll");
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        Definitions.Add("OPENGLRHI_EXPORTS");
    }
}
