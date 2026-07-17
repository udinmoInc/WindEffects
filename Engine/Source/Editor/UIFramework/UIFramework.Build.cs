using IgniteBT.BuildSystem;
using System.IO;

public class UIFramework : ModuleRules
{
    public UIFramework(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects/Editor/UI");
        PublicIncludePaths.Add("Public/WindEffects/Editor");
        // Propagate Runtime UI headers to consumers that still include via UIFramework shims.
        PublicIncludePaths.Add(Path.Combine(context.EngineDirectory, "Source", "Runtime", "UI", "Public"));
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("UI");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("Icons");

        // Editor shell still needs engine world/scene for some panel integrations.
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");

        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");

        Definitions.Add("UIFRAMEWORK_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();

        if (string.Equals(context.Configuration, "Debug", System.StringComparison.OrdinalIgnoreCase))
        {
            PlatformSettings.Windows.CompilerFlags.Add("/MD");
            PlatformSettings.Windows.CompilerFlags.Add("/D_ITERATOR_DEBUG_LEVEL=0");
        }
    }
}
