using IgniteBT.BuildSystem;
using System.IO;

public class Icons : ModuleRules
{
    public Icons(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PublicIncludePaths.Add("Public/WindEffects");
        PublicIncludePaths.Add("Public/WindEffects/Runtime");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");

        var thirdPartyRoot = Path.Combine(context.EngineDirectory, "ThirdParty");
        PrivateIncludePaths.Add(Path.Combine(thirdPartyRoot, "stb"));

        Definitions.Add("ICONS_EXPORTS");

        PlatformSettings.Windows ??= new WindowsSettings();
    }
}
