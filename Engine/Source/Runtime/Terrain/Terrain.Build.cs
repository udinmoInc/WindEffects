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

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("RHI");
        PrivateDependencies.Add("Renderer");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("TERRAIN_EXPORTS");
    }
}
