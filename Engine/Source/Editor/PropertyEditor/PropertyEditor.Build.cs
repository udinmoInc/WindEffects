using IgniteBT.BuildSystem;

public class PropertyEditor : ModuleRules
{
    public PropertyEditor(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("UIFramework");
        PrivateDependencies.Add("ContentBrowser"); // SearchBox used by Details panel

        Definitions.Add("PROPERTYEDITOR_EXPORTS");
    }
}
