using IgniteBT.BuildSystem;

public class CoreUObject : ModuleRules
{
    public CoreUObject(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");

        Definitions.Add("COREUOBJECT_EXPORTS");
    }
}
