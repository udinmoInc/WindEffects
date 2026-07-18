#pragma once

#include "Reflection/NameId.h"

#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace we::runtime::reflection {

class NameTable final : public INameTable {
public:
    NameTable();

    [[nodiscard]] NameId Intern(std::string_view text) override;
    [[nodiscard]] std::string_view Resolve(NameId id) const noexcept override;
    [[nodiscard]] bool Contains(std::string_view text) const override;
    [[nodiscard]] std::size_t GetCount() const override;
    [[nodiscard]] std::size_t GetByteSize() const override;

private:
    mutable std::shared_mutex m_Mutex;
    std::vector<std::string> m_Storage; // index 0 unused (kInvalidNameId)
    std::unordered_map<std::string, NameId> m_Lookup;
    std::size_t m_ByteSize = 0;
};

} // namespace we::runtime::reflection
