using IgniteBT.BuildSystem;
using System.IO;

public class KindUI : ModuleRules
{
    public KindUI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("Icons");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "lunasvg", "include"));

        RequiredThirdParty.Add("lunasvg");

        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");

        Definitions.Add("WE_HAS_LUNASVG=1");
        Definitions.Add("KINDUI_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();

        var configName = context.Configuration ?? string.Empty;
        var lunaConfigLibDir = Path.Combine(thirdPartyRoot, "lunasvg", "lib", configName);
        var lunaLibDir = Directory.Exists(lunaConfigLibDir)
            ? lunaConfigLibDir
            : Path.Combine(thirdPartyRoot, "lunasvg", "lib");
        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{lunaLibDir}\"");
        PlatformSettings.Windows.LinkerFlags.Add("lunasvg.lib");

        if (string.Equals(context.Configuration, "Debug", System.StringComparison.OrdinalIgnoreCase))
        {
            PlatformSettings.Windows.CompilerFlags.Add("/MD");
            PlatformSettings.Windows.CompilerFlags.Add("/D_ITERATOR_DEBUG_LEVEL=0");
        }
    }
}
