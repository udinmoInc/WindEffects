// ==============================================================================
// WindEffects — Text — TextModule
// Internal implementation for the Text module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#include "Modules/IModuleInterface.h"
#include "Core/Logger.h"

class TextModule : public we::core::IModuleInterface
{
public:
    void StartupModule() override
    {
        WE_LOG_TRACE("Text", "TextModule started");
    }

    void ShutdownModule() override
    {
        WE_LOG_TRACE("Text", "TextModule shutdown");
    }
};

IMPLEMENT_MODULE(TextModule, WindEffects_Text)
