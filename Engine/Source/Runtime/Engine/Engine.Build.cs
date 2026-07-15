using IgniteBT.BuildSystem;

public class Engine : ModuleRules
{
    public Engine(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WindeffectsEngine.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Platform");

        AddOptionalThirdParty("glm");

        Definitions.Add("ENGINE_EXPORTS");
    }
}
