#include "Reflection/TypeId.h"
#include "HashUtils.h"

namespace we::runtime::reflection {

TypeId HashTypeName(std::string_view qualifiedName) noexcept {
    return detail::Fnv1a64(qualifiedName);
}

} // namespace we::runtime::reflection
