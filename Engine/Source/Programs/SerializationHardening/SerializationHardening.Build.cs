// ==============================================================================
// WindEffects — SerializationHardening — SerializationHardening.Build
// Source file for the SerializationHardening module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class SerializationHardening : ModuleRules
{
    public SerializationHardening(ModuleContext context) : base(context)
    {
        Type = ModuleType.Executable;

        SetBinaryName("SerializationHardening.exe");
        PublishAtConfigurationRoot();

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");

        PlatformSettings.Windows ??= new WindowsSettings();
        PlatformSettings.Windows.Subsystem = "Console";
    }
}
