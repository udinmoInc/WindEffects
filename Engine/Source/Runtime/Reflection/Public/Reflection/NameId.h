#pragma once

#include "Reflection/Export.h"

#include <cstdint>
#include <string_view>

namespace we::runtime::reflection {

/// Interned immutable string identifier. Prefer NameId over string compares in hot paths.
using NameId = std::uint32_t;
inline constexpr NameId kInvalidNameId = 0;

/// Process-wide name / metadata-key intern table (thread-safe, append-mostly).
class REFLECTION_API INameTable {
public:
    virtual ~INameTable() = default;

    /// Intern a string; identical content always returns the same NameId.
    [[nodiscard]] virtual NameId Intern(std::string_view text) = 0;

    /// Resolve a NameId to a stable string_view into immutable storage.
    /// Valid for the lifetime of the process (entries are never erased).
    [[nodiscard]] virtual std::string_view Resolve(NameId id) const noexcept = 0;

    [[nodiscard]] virtual bool Contains(std::string_view text) const = 0;
    [[nodiscard]] virtual std::size_t GetCount() const = 0;
    [[nodiscard]] virtual std::size_t GetByteSize() const = 0;
};

[[nodiscard]] REFLECTION_API INameTable& GetNameTable();
[[nodiscard]] REFLECTION_API NameId InternName(std::string_view text);
[[nodiscard]] REFLECTION_API std::string_view ResolveName(NameId id) noexcept;

} // namespace we::runtime::reflection
