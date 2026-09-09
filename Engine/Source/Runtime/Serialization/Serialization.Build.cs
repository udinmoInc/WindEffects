// ==============================================================================
// WindEffects — Serialization — Serialization.Build
// Source file for the Serialization module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class Serialization : ModuleRules
{
    public Serialization(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Reflection");

        Definitions.Add("SERIALIZATION_EXPORTS");
    }
}
