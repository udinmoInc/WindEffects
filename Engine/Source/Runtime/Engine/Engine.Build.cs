using IgniteBT.BuildSystem;

public class Engine : ModuleRules
{
    public Engine(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");

        Definitions.Add("ENGINE_EXPORTS");
    }
}
