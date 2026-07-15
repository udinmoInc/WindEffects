using IgniteBT.BuildSystem;
using System.IO;

public class Terrain : ModuleRules
{
    public Terrain(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add("Public/WindEffects/Runtime");
        PrivateIncludePaths.Add("Private");
        PrivateIncludePaths.Add(Path.Combine(context.EngineDirectory, "ThirdParty", "stb"));

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("RHI");

        AddOptionalThirdParty("glm");

        Definitions.Add("TERRAIN_EXPORTS");
    }
}
