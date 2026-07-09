using IgniteBT.BuildSystem;
using System.IO;

public class Application : ModuleRules
{
    public Application(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");

        OptionalSDK("VulkanSDK");
        OptionalSDK("SDL3");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "freetype", "include"));
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "msdf-atlas-gen"));
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "msdf-atlas-gen", "msdfgen"));
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "lunasvg", "include"));

        RequiredThirdParty.Add("freetype");
        RequiredThirdParty.Add("msdf-atlas-gen");
        RequiredThirdParty.Add("lunasvg");

        Definitions.Add("WE_HAS_FREETYPE=1");
        Definitions.Add("WE_HAS_MSDFGEN=1");
        Definitions.Add("WE_HAS_LUNASVG=1");
        Definitions.Add("APPLICATION_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();

        var freetypeLibDir = Path.Combine(thirdPartyRoot, "freetype", "lib");
        var msdfLibDir = Path.Combine(thirdPartyRoot, "msdf-atlas-gen", "lib");
        var lunaLibDir = Path.Combine(thirdPartyRoot, "lunasvg", "lib");

        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{freetypeLibDir}\"");
        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{msdfLibDir}\"");
        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{lunaLibDir}\"");

        PlatformSettings.Windows.LinkerFlags.Add("freetype.lib");
        PlatformSettings.Windows.LinkerFlags.Add("msdfgen-core.lib");
        PlatformSettings.Windows.LinkerFlags.Add("msdfgen-ext.lib");
        PlatformSettings.Windows.LinkerFlags.Add("msdf-atlas-gen.lib");
        PlatformSettings.Windows.LinkerFlags.Add("lunasvg.lib");
    }
}
