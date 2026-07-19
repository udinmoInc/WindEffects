using IgniteBT.BuildSystem;

public class Undo : ModuleRules
{
    public Undo(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Transaction framework — Reflection metadata + Serialization diffs/snapshots only.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("World");
        PublicDependencies.Add("PropertyEditor"); // IPropertyTransactionHook adapter

        Definitions.Add("UNDO_EXPORTS");
    }
}
