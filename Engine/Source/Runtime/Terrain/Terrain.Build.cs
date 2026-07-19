using IgniteBT.BuildSystem;
using System.IO;

public class Terrain : ModuleRules
{
    public Terrain(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PrivateIncludePaths.Add(Path.Combine(context.EngineDirectory, "ThirdParty", "stb"));

        // Terrain Runtime — never owns World lifetime, Reflection schemas, or Undo history.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("ECS");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("AssetRuntime");
        PrivateDependencies.Add("Renderer");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("TERRAIN_EXPORTS");
    }
}
