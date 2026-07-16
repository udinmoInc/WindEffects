using IgniteBT.BuildSystem;

public class WeLauncher : ModuleRules
{
    public WeLauncher(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WeLauncher.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add(".");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("CoreUObject");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("ECS");
        PublicDependencies.Add("UIFramework");

        PrivateDependencies.Add("VulkanRHI");
        PrivateDependencies.Add("NullRHI");

        AddOptionalThirdParty("nlohmann_json");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Windows";
    }
}
