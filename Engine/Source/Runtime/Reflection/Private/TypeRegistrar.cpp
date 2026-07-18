#include "Reflection/ITypeRegistrar.h"

namespace we::runtime::reflection {

void ApplyTypeRegistrar(ITypeRegistrar& registrar) {
    registrar.RegisterTypes(GetTypeRegistry());
}

void RemoveTypeRegistrar(ITypeRegistrar& registrar) {
    registrar.UnregisterTypes(GetTypeRegistry());
}

} // namespace we::runtime::reflection
