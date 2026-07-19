using IgniteBT.BuildSystem;

public class Compilation : ModuleRules
{
    public Compilation(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Single authoritative compile pipeline — never owned by Materials/Shaders/VFX/Editor.
        // Does not depend on Renderer/RHI backends or Editor modules.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetRuntime");

        Definitions.Add("COMPILATION_EXPORTS");
    }
}
