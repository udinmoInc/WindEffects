using IgniteBT.BuildSystem;

public class RHI : ModuleRules
{
    public RHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");

        PublicIncludePaths.Add(System.IO.Path.Combine(context.EngineDirectory, "ThirdParty", "Vulkan-Headers", "include"));

        Definitions.Add("RHI_EXPORTS");
    }
}
