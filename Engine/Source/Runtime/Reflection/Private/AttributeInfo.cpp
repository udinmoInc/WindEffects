#include "Reflection/AttributeInfo.h"

namespace we::runtime::reflection {

AttributeInfo AttributeInfo::MakeBool(std::string_view name, bool value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.kind = AttributeValueKind::Bool;
    a.boolValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeInt(std::string_view name, std::int64_t value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.kind = AttributeValueKind::Int64;
    a.intValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeFloat(std::string_view name, double value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.kind = AttributeValueKind::Double;
    a.floatValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeString(std::string_view name, std::string_view value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.kind = AttributeValueKind::String;
    a.stringValue = std::string(value);
    return a;
}

AttributeInfo AttributeInfo::MakeTypeId(std::string_view name, TypeId value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.kind = AttributeValueKind::TypeId;
    a.typeIdValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeBytes(std::string_view name, const std::uint8_t* data, std::size_t size) {
    AttributeInfo a;
    a.name = std::string(name);
    a.kind = AttributeValueKind::Bytes;
    if (data && size > 0) {
        a.bytesValue.assign(data, data + size);
    }
    return a;
}

void AttributeBag::Add(AttributeInfo attr) {
    if (attr.name.empty()) {
        return;
    }
    for (AttributeInfo& entry : entries) {
        if (entry.name == attr.name) {
            entry = std::move(attr);
            return;
        }
    }
    entries.push_back(std::move(attr));
}

const AttributeInfo* AttributeBag::Find(std::string_view name) const noexcept {
    for (const AttributeInfo& entry : entries) {
        if (entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace we::runtime::reflection
