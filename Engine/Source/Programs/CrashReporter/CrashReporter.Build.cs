using IgniteBT.BuildSystem;

public class CrashReporter : ModuleRules
{
    public CrashReporter(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("WECrashReporter.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PrivateDependencies.Add("Engine");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("KindUI");
        PrivateDependencies.Add("UIFramework");

        AddOptionalThirdParty("nlohmann_json");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Windows";
    }
}
