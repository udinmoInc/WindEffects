using IgniteBT.BuildSystem;

public class SerializationHardening : ModuleRules
{
    public SerializationHardening(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("SerializationHardening.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
