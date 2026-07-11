using IgniteBT.BuildSystem;

public class CrashReporter : ModuleRules
{
    public CrashReporter(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WECrashReporter.exe");
        PublishAtConfigurationRoot();

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("Application");
        PublicDependencies.Add("UIFramework");

        AddOptionalThirdParty("nlohmann_json");

        OptionalSDK("VulkanSDK");
        OptionalSDK("SDL3");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");
        DefineIf(!HasSDK("VulkanSDK"), "WE_HAS_VULKAN=0");
        DefineIf(HasSDK("SDL3"), "WE_HAS_SDL3=1");
        DefineIf(!HasSDK("SDL3"), "WE_HAS_SDL3=0");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Windows";
    }
}
