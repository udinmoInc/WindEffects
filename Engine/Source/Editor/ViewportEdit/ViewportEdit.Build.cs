// ==============================================================================
// WindEffects — ViewportEdit — ViewportEdit.Build
// Source file for the ViewportEdit module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
using IgniteBT.BuildSystem;

public class ViewportEdit : ModuleRules
{
    public ViewportEdit(ModuleContext context) : base(context)
    {
        Type = ModuleType.SharedLibrary;

        PublicIncludePaths.Add("Public");
        PrivateIncludePaths.Add("Private");

        // Interaction layer only — World/Scene own objects; Undo owns history; Renderer owns pixels.
        // Does NOT depend on Viewport widget (Viewport→PlaceActors→TerrainEditor would cycle).
        PublicDependencies.Add("Core");
        PublicDependencies.Add("Platform");
        PublicDependencies.Add("Engine");
        PublicDependencies.Add("Scene");
        PublicDependencies.Add("World");
        PublicDependencies.Add("Reflection");
        PublicDependencies.Add("Serialization");
        PublicDependencies.Add("Undo");
        PublicDependencies.Add("PropertyEditor");
        PublicDependencies.Add("KindUI");
        PublicDependencies.Add("EditorShell");

        PrivateDependencies.Add("RHI");
        PrivateDependencies.Add("Renderer");
        PrivateDependencies.Add("Terrain");

        AddOptionalThirdParty("glm");
        DefineIf(HasThirdParty("glm"), "WE_HAS_GLM=1");
        DefineIf(!HasThirdParty("glm"), "WE_HAS_GLM=0");

        Definitions.Add("VIEWPORTEDIT_EXPORTS");
    }
}
