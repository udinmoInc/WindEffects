// ==============================================================================
// WindEffects — RHI — RHI.Build
// Source file for the RHI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class RHI : ModuleRules
{
    public RHI(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        BootstrapBinary();
        SetBinaryName("WERHI.dll");

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");

        // Public headers must stay free of Vulkan / DX / Metal / GL.
        Definitions.Add("RHI_EXPORTS");
    }
}
