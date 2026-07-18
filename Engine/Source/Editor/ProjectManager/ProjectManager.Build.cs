using IgniteBT.BuildSystem;

public class ProjectManager : ModuleRules
{
    public ProjectManager(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Projects");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("UIFramework");

        Definitions.Add("PROJECTMANAGER_EXPORTS");
    }
}
