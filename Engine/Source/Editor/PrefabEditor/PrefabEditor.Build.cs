// ==============================================================================
// WindEffects — PrefabEditor — PrefabEditor.Build
// Source file for the PrefabEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class PrefabEditor : ModuleRules
{
    public PrefabEditor(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Editor glue over Prefab Runtime — Undo via injected callback (no Undo module link).
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Prefab");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");

        Definitions.Add("PREFABEDITOR_EXPORTS");
    }
}
