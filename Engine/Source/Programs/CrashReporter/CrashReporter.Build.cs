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
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("UI");
        PublicDependencies.Add("UIFramework");

        AddOptionalThirdParty("nlohmann_json");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Windows";
    }
}
