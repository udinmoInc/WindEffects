// ==============================================================================
// WindEffects — WorldOutliner — WorldOutliner.Build
// Source file for the WorldOutliner module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class WorldOutliner : ModuleRules
{
    public WorldOutliner(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Presentation/interaction over World/Scene — never owns gameplay data.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("Undo");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("ViewportEdit");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("Text");
        PublicDependencies.Add("EditorShell");
        PublicDependencies.Add("ContentBrowser");

        PrivateDependencies.Add("RHI");

        Definitions.Add("WORLDOUTLINER_EXPORTS");
    }
}
