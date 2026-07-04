using IgniteBT.BuildSystem;

public class CoreUObject : ModuleRules
{
    public CoreUObject(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WindeffectsCoreUObject.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");

        Definitions.Add("COREUOBJECT_EXPORTS");
    }
}
