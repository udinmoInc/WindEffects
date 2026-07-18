using IgniteBT.BuildSystem;

public class ECSTests : ModuleRules
{
    public ECSTests(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("ECSTests.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("ECS");
        PublicDependencies.Add("Scene");

        AddOptionalThirdParty("glm");
        Definitions.Add("WE_HAS_GLM=1");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
