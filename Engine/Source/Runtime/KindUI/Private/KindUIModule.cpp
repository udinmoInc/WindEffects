// ==============================================================================
// WindEffects — KindUI — KindUIModule
// Internal implementation for the KindUI module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class KindUIModule : public we::core::IModuleInterface
{
public:
    void StartupModule() override
    {
        WE_LOG_TRACE("KindUI", "KindUIModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("KindUI", "KindUIModule shutdown");
    }
};

IMPLEMENT_MODULE(KindUIModule, WindEffects_KindUI)
 
