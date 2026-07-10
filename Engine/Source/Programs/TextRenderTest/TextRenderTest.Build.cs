using IgniteBT.BuildSystem;

public class TextRenderTest : ModuleRules
{
    public TextRenderTest(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("TextRenderTest.exe");
        PublishAtConfigurationRoot();

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Text");

        OptionalSDK("VulkanSDK");
        DefineIf(HasSDK("VulkanSDK"), "WE_HAS_VULKAN=1");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
