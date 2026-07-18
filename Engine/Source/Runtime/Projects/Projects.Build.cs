using IgniteBT.BuildSystem;

public class Projects : ModuleRules
{
    public Projects(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");

        Definitions.Add("PROJECTS_EXPORTS");

        AddOptionalThirdParty("nlohmann_json");
        DefineIf(HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=1");
        DefineIf(!HasThirdParty("nlohmann_json"), "WE_HAS_NLOHMANN_JSON=0");
    }
}
