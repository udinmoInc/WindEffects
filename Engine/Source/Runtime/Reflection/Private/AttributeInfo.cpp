#include "Reflection/AttributeInfo.h"
#include "Reflection/NameId.h"

namespace we::runtime::reflection {

AttributeInfo AttributeInfo::MakeBool(std::string_view name, bool value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.nameId = InternName(name);
    a.kind = AttributeValueKind::Bool;
    a.boolValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeInt(std::string_view name, std::int64_t value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.nameId = InternName(name);
    a.kind = AttributeValueKind::Int64;
    a.intValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeFloat(std::string_view name, double value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.nameId = InternName(name);
    a.kind = AttributeValueKind::Double;
    a.floatValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeString(std::string_view name, std::string_view value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.nameId = InternName(name);
    a.kind = AttributeValueKind::String;
    a.stringValue = std::string(value);
    a.stringValueId = InternName(value);
    return a;
}

AttributeInfo AttributeInfo::MakeTypeId(std::string_view name, TypeId value) {
    AttributeInfo a;
    a.name = std::string(name);
    a.nameId = InternName(name);
    a.kind = AttributeValueKind::TypeId;
    a.typeIdValue = value;
    return a;
}

AttributeInfo AttributeInfo::MakeBytes(std::string_view name, const std::uint8_t* data, std::size_t size) {
    AttributeInfo a;
    a.name = std::string(name);
    a.nameId = InternName(name);
    a.kind = AttributeValueKind::Bytes;
    if (data && size > 0) {
        a.bytesValue.assign(data, data + size);
    }
    return a;
}

void AttributeBag::Add(AttributeInfo attr) {
    EnsureLoaded();
    if (attr.nameId == kInvalidNameId && !attr.name.empty()) {
        attr.nameId = InternName(attr.name);
    }
    if (attr.name.empty() && attr.nameId == kInvalidNameId) {
        return;
    }
    for (AttributeInfo& entry : entries) {
        if ((attr.nameId != kInvalidNameId && entry.nameId == attr.nameId)
            || (!attr.name.empty() && entry.name == attr.name)) {
            entry = std::move(attr);
            return;
        }
    }
    entries.push_back(std::move(attr));
}

void AttributeBag::SetLazyLoader(AttributeLazyLoadFn loader) {
    lazyLoader = loader;
    loaded = loader == nullptr;
}

void AttributeBag::EnsureLoaded() const {
    if (loaded) {
        return;
    }
    if (lazyLoader) {
        lazyLoader(const_cast<AttributeBag&>(*this));
    }
    loaded = true;
}

const AttributeInfo* AttributeBag::Find(std::string_view name) const noexcept {
    EnsureLoaded();
    for (const AttributeInfo& entry : entries) {
        if (entry.name == name) {
            return &entry;
        }
    }
    return nullptr;
}

const AttributeInfo* AttributeBag::Find(NameId nameId) const noexcept {
    EnsureLoaded();
    if (nameId == kInvalidNameId) {
        return nullptr;
    }
    for (const AttributeInfo& entry : entries) {
        if (entry.nameId == nameId) {
            return &entry;
        }
    }
    return nullptr;
}

} // namespace we::runtime::reflection
