using IgniteBT.BuildSystem;

public class Renderer : ModuleRules
{
    public Renderer(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add("Public/WindEffects/Runtime");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("volk");
        PublicDependencies.Add("vma");

        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK") || true, "WE_HAS_VULKAN=1");

        PublicIncludePaths.Add(System.IO.Path.Combine(context.EngineDirectory, "ThirdParty", "Vulkan-Headers", "include"));

        AddOptionalThirdParty("glm");

        Definitions.Add("RENDERER_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.LinkerFlags.Add("dbghelp.lib");
    }
}
