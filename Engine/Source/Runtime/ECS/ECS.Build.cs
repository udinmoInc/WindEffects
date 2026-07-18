using IgniteBT.BuildSystem;

public class ECS : ModuleRules
{
    public ECS(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");

        AddOptionalThirdParty("glm");
        // Match Engine/Scene ABI — TransformComponent always uses glm.
        Definitions.Add("WE_HAS_GLM=1");

        Definitions.Add("ECS_EXPORTS");
    }
}
