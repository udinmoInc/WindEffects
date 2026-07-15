using IgniteBT.BuildSystem;
using System.IO;

public class VulkanRHI : ModuleRules
{
    public VulkanRHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WEVulkanRHI.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");
        PrivateIncludePaths.Add("Private/Scene");
        PrivateIncludePaths.Add("Private/Scene/Public");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PrivateDependencies.Add("Engine");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("Scene");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PrivateIncludePaths.Add(Path.Combine(thirdPartyRoot, "volk"));
        PrivateIncludePaths.Add(Path.Combine(thirdPartyRoot, "Vulkan-Headers", "include"));

        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK") || true, "WE_HAS_VULKAN=1");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("VULKANRHI_EXPORTS");
        Definitions.Add("RENDERER_EXPORTS"); // legacy symbols moved from Renderer during cutover

        PlatformSettings.Windows ??= new WindowsSettings();
    }
}
