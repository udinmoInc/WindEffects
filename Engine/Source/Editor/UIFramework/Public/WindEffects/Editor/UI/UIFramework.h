#pragma once

// WindEffects Editor UI Framework — public entry point
#include "WindEffects/Editor/UI/Core/EditorApplicationContext.h"
#include "WindEffects/Editor/UI/Core/IServiceProvider.h"
#include "WindEffects/Editor/UI/Core/ServiceContainer.h"
#include "WindEffects/Editor/UI/Core/Types.h"
#include "WindEffects/Editor/UI/Theming/IThemeProvider.h"
#include "WindEffects/Editor/UI/Theming/GraphiteDarkTheme.h"
#include "WindEffects/Editor/UI/Resources/IResourceRegistry.h"
#include "WindEffects/Editor/UI/Events/IEventBus.h"
#include "WindEffects/Editor/UI/Commands/ICommandRegistry.h"
#include "WindEffects/Editor/UI/Docking/IDockManager.h"
#include "WindEffects/Editor/UI/Docking/DockManager.h"
#include "WindEffects/Editor/UI/Extensions/UIExtensionRegistry.h"
#include "WindEffects/Editor/UI/Shell/DockLayoutBuilder.h"
