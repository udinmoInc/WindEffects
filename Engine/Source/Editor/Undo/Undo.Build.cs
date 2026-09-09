// ==============================================================================
// WindEffects — Undo — Undo.Build
// Source file for the Undo module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
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
