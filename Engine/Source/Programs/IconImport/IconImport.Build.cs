using IgniteBT.BuildSystem;

public class IconImport : ModuleRules
{
    public IconImport(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("we-icon-compile.exe");
        PublishAtConfigurationRoot();

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Icons");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
