using IgniteBT.BuildSystem;

public class TerrainEditor : ModuleRules
{
    public TerrainEditor(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");
        PublicDependencies.Add("Terrain");
        // ToolsPanel was unused — removed.

        AddOptionalThirdParty("glm");

        Definitions.Add("TERRAINEDITOR_EXPORTS");
    }
}
