// ==============================================================================
// WindEffects — TerrainEditor — TerrainEditor.Build
// Source file for the TerrainEditor module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class TerrainEditor : ModuleRules
{
    public TerrainEditor(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("RHI");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Renderer");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("EditorShell");
        PublicDependencies.Add("Terrain");
        PublicDependencies.Add("ViewportEdit");
        PublicDependencies.Add("Undo");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("PropertyEditor");

        AddOptionalThirdParty("glm");

        Definitions.Add("TERRAINEDITOR_EXPORTS");
    }
}
