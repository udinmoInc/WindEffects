using IgniteBT.BuildSystem;

public class Renderer : ModuleRules
{
    public Renderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("volk");
        PublicDependencies.Add("vma");

        // Add Vulkan SDK as optional dependency
        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK") || true, "WE_HAS_VULKAN=1");

        // Force fallback to ThirdParty Vulkan-Headers
        PublicIncludePaths.Add(System.IO.Path.Combine(context.EngineDirectory, "ThirdParty", "Vulkan-Headers", "include"));

        // SDL3 is used for Vulkan surface creation and window sizing
        OptionalSDK("SDL3");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        AddOptionalThirdParty("glm");

        Definitions.Add("RENDERER_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("dbghelp.lib");
    }
}
