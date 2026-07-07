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

        // WE_HAS_GLM comes from global feature flags when glm is bootstrapped.
        // Never force WE_HAS_GLM=0 here — it overrides the global flag and breaks EditorCamera ABI.
        AddOptionalThirdParty("glm");

        OptionalSDK("SDL3");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        Definitions.Add("ENGINE_EXPORTS");
    }
}
