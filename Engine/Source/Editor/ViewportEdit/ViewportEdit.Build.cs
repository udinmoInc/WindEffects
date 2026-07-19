using IgniteBT.BuildSystem;

public class ViewportEdit : ModuleRules
{
    public ViewportEdit(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Interaction layer only — World/Scene own objects; Undo owns history; Renderer owns pixels.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("Undo");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("Viewport");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("UIFramework");

        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("ToolsPanel");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("VIEWPORTEDIT_EXPORTS");
    }
}
