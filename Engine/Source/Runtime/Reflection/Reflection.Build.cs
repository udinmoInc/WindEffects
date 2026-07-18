using IgniteBT.BuildSystem;

public class Reflection : ModuleRules
{
    public Reflection(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");

        Definitions.Add("REFLECTION_EXPORTS");
    }
}
