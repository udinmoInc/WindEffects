using IgniteBT.BuildSystem;
using System.IO;

public class Text : ModuleRules
{
    public Text(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Renderer");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "freetype", "include"));
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "msdf-atlas-gen"));
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "msdf-atlas-gen", "msdfgen"));
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "harfbuzz", "src"));

        RequiredThirdParty.Add("freetype");
        RequiredThirdParty.Add("msdf-atlas-gen");
        RequiredThirdParty.Add("harfbuzz");

        Definitions.Add("WE_HAS_FREETYPE=1");
        Definitions.Add("WE_HAS_MSDFGEN=1");
        Definitions.Add("WE_HAS_HARFBUZZ=1");
        Definitions.Add("TEXT_EXPORTS");

        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");

        PlatformSettings.Windows ??= new WindowsSettings();

        var freetypeLibDir = Path.Combine(thirdPartyRoot, "freetype", "lib");
        var msdfLibDir = Path.Combine(thirdPartyRoot, "msdf-atlas-gen", "lib");
        var harfBuzzLibDir = Path.Combine(thirdPartyRoot, "harfbuzz", "lib");

        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{freetypeLibDir}\"");
        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{msdfLibDir}\"");
        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{harfBuzzLibDir}\"");

        PlatformSettings.Windows.LinkerFlags.Add("freetype.lib");
        PlatformSettings.Windows.LinkerFlags.Add("msdfgen-core.lib");
        PlatformSettings.Windows.LinkerFlags.Add("msdfgen-ext.lib");
        PlatformSettings.Windows.LinkerFlags.Add("msdf-atlas-gen.lib");
        PlatformSettings.Windows.LinkerFlags.Add("harfbuzz.lib");
    }
}
