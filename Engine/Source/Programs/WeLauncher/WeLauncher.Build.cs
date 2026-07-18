using IgniteBT.BuildSystem;

public class WeLauncher : ModuleRules
{
    public WeLauncher(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WeLauncher.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("Icons");
        PrivateDependencies.Add("Engine");
        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("ECS");

        PrivateDependencies.Add("VulkanRHI");
        PrivateDependencies.Add("NullRHI");

        AddOptionalThirdParty("nlohmann_json");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Windows";
    }
}
