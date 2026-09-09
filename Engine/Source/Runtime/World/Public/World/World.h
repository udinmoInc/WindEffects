// ==============================================================================
// WindEffects — World — World
// Public API surface for the World module.
//
// Copyright (c) 2026 WindEffects. All rights reserved.
// This file is part of WindEffects Engine and is governed by the
// WindEffects Engine EULA (see Legal/EULA.md at the repository root).
// ==============================================================================
#pragma once

// WindEffects World Runtime — umbrella public header.

#include "World/Export.h"
#include "World/WorldGuid.h"
#include "World/WorldTypes.h"
#include "World/WorldDiagnostics.h"
#include "World/IWorld.h"
#include "World/IWorldContext.h"
#include "World/IWorldManager.h"
#include "World/IWorldRegistry.h"
#include "World/IWorldRuntime.h"
#include "World/ILevel.h"
#include "World/IActor.h"
#include "World/IHierarchy.h"
#include "World/ITransformHierarchy.h"
#include "World/ITagLayer.h"
#include "World/IQuery.h"
#include "World/ITickSystem.h"
#include "World/IStreaming.h"
#include "World/IWorldPersistence.h"
#include "World/IPrefabOrigin.h"
#include "World/IWorldSubsystem.h"
#include "World/WorldReflection.h"
#include "World/Integration/IWorldHooks.h"
#include "World/WorldTests.h"
#include "World/WorldBenchmark.h"
