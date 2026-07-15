using IgniteBT.BuildSystem;

public class ECS : ModuleRules
{
    public ECS(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add("Public/WindEffects/Runtime");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");

        AddOptionalThirdParty("glm");

        Definitions.Add("ECS_EXPORTS");
    }
}
