using IgniteBT.BuildSystem;

public class World : ModuleRules
{
    public World(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Orchestration layer over Scene/ECS; Reflection + Serialization for metadata persistence.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("ECS");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");

        // Streaming uses AssetRuntime; Renderer sync remains environment-private.
        PrivateDependencies.Add("AssetRuntime");
        PrivateDependencies.Add("AssetImporter");
        PrivateDependencies.Add("Renderer");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("WORLD_EXPORTS");
    }
}
