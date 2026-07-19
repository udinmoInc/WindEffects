#pragma once

/// Thin include wrapper — Environment and EditorShell keep `#include "Widgets/PropertyEditor.h"`.
/// Implementation lives in Compat/LegacyPropertyEditor (shim over / beside the new stack).
#include "PropertyEditor/Compat/LegacyPropertyEditor.h"
