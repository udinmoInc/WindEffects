using IgniteBT.BuildSystem;

public class EditorShell : ModuleRules
{
    public EditorShell(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        // Public includes keep WindEffects/Editor/... paths for panel authors.
        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("Icons");
        PrivateDependencies.Add("Engine");
        PrivateDependencies.Add("Scene");
        PrivateDependencies.Add("World");
        PrivateDependencies.Add("Menus");

        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");

        Definitions.Add("EDITORSHELL_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();

        if (string.Equals(context.Configuration, "Debug", System.StringComparison.OrdinalIgnoreCase))
        {
            PlatformSettings.Windows.CompilerFlags.Add("/MD");
            PlatformSettings.Windows.CompilerFlags.Add("/D_ITERATOR_DEBUG_LEVEL=0");
        }
    }
}
