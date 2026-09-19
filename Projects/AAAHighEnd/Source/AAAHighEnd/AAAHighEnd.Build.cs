using IgniteBT.BuildSystem;

public class AAAHighEnd : ModuleRules
{
    public AAAHighEnd(ModuleContext context) : base(context)
    {
        Type = ModuleType.GameModule;
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Engine");
    }
}
