using IgniteBT.BuildSystem;

public class Engine : ModuleRules
{
    public Engine(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        Dependencies.Add("Core");
        Dependencies.Add("CoreUObject");

        // Add GLM as optional third-party dependency
        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("ENGINE_EXPORTS");
    }
}
