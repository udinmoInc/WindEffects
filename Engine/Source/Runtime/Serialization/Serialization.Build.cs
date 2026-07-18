using IgniteBT.BuildSystem;

public class Serialization : ModuleRules
{
    public Serialization(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Reflection");

        Definitions.Add("SERIALIZATION_EXPORTS");
    }
}
