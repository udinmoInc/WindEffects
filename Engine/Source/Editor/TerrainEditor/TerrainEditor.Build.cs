using IgniteBT.BuildSystem;

public class TerrainEditor : ModuleRules
{
    public TerrainEditor(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("ToolsPanel");
        PublicDependencies.Add("Terrain");

        AddOptionalThirdParty("glm");

        Definitions.Add("TERRAINEDITOR_EXPORTS");
    }
}
