using IgniteBT.BuildSystem;

public class ReflectionHardening : ModuleRules
{
    public ReflectionHardening(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("ReflectionHardening.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Reflection");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
