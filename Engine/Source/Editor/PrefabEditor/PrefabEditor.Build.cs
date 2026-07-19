using IgniteBT.BuildSystem;

public class PrefabEditor : ModuleRules
{
    public PrefabEditor(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Editor glue over Prefab Runtime — Undo via injected callback (no Undo module link).
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Prefab");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");

        Definitions.Add("PREFABEDITOR_EXPORTS");
    }
}
