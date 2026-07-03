using IgniteBT.BuildSystem;

public class Application : ModuleRules
{
    public Application(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");

        OptionalSDK("SDL3");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        Definitions.Add("APPLICATION_EXPORTS");
    }
}
