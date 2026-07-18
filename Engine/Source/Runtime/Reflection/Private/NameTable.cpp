#include "NameTable.h"

#include "HashUtils.h"

namespace we::runtime::reflection {

NameTable::NameTable() {
    m_Storage.emplace_back(); // id 0 = invalid
}

NameId NameTable::Intern(std::string_view text) {
    if (text.empty()) {
        return kInvalidNameId;
    }

    {
        std::shared_lock readLock(m_Mutex);
        const auto it = m_Lookup.find(std::string(text));
        if (it != m_Lookup.end()) {
            return it->second;
        }
    }

    std::unique_lock writeLock(m_Mutex);
    const auto it = m_Lookup.find(std::string(text));
    if (it != m_Lookup.end()) {
        return it->second;
    }

    const NameId id = static_cast<NameId>(m_Storage.size());
    m_Storage.emplace_back(text);
    m_ByteSize += text.size();
    m_Lookup.emplace(m_Storage.back(), id);
    return id;
}

std::string_view NameTable::Resolve(NameId id) const noexcept {
    std::shared_lock lock(m_Mutex);
    if (id == kInvalidNameId || id >= m_Storage.size()) {
        return {};
    }
    return m_Storage[id];
}

bool NameTable::Contains(std::string_view text) const {
    std::shared_lock lock(m_Mutex);
    return m_Lookup.find(std::string(text)) != m_Lookup.end();
}

std::size_t NameTable::GetCount() const {
    std::shared_lock lock(m_Mutex);
    return m_Storage.size() > 0 ? m_Storage.size() - 1 : 0;
}

std::size_t NameTable::GetByteSize() const {
    std::shared_lock lock(m_Mutex);
    return m_ByteSize;
}

INameTable& GetNameTable() {
    static NameTable table;
    return table;
}

NameId InternName(std::string_view text) {
    return GetNameTable().Intern(text);
}

std::string_view ResolveName(NameId id) noexcept {
    return GetNameTable().Resolve(id);
}

} // namespace we::runtime::reflection
