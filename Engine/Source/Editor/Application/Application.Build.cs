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
        PublicDependencies.Add("Text");

        OptionalSDK("VulkanSDK");
        OptionalSDK("SDL3");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PublicIncludePaths.Add(Path.Combine(thirdPartyRoot, "lunasvg", "include"));

        RequiredThirdParty.Add("lunasvg");

        Definitions.Add("WE_HAS_LUNASVG=1");
        Definitions.Add("APPLICATION_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();

        var lunaLibDir = Path.Combine(thirdPartyRoot, "lunasvg", "lib");

        PlatformSettings.Windows.LinkerFlags.Add($"/LIBPATH:\"{lunaLibDir}\"");
        PlatformSettings.Windows.LinkerFlags.Add("lunasvg.lib");
    }
}
