// ==============================================================================
// WindEffects — Prefab — Prefab.Build
// Source file for the Prefab module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class Prefab : ModuleRules
{
    public Prefab(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Prefab assets & instancing — never owns World lifetime, Reflection schemas, or Undo.
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("ECS");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("AssetImporter");
        PublicDependencies.Add("AssetRuntime");

        Definitions.Add("PREFAB_EXPORTS");
    }
}
