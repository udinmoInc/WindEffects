using IgniteBT.BuildSystem;

public class TextRenderTest : ModuleRules
{
    public TextRenderTest(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("TextRenderTest.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Text");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
