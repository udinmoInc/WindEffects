using IgniteBT.BuildSystem;

public class FontImport : ModuleRules
{
    public FontImport(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("we-font-import.exe");
        PublishAtConfigurationRoot();

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Text");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
