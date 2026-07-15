#pragma once

#include "RHI/IRHI.h"
#include "RHI/Types.h"

#include <utility>

namespace we::rhi {

// RAII buffer destroy with deferred destruction on the owning device.
class ScopedBuffer {
public:
    ScopedBuffer() = default;
    ScopedBuffer(IRHIDevice* device, RHIBufferHandle handle) noexcept
        : m_Device(device)
        , m_Handle(handle)
    {
    }

    ~ScopedBuffer() { Reset(); }

    ScopedBuffer(ScopedBuffer&& other) noexcept
        : m_Device(other.m_Device)
        , m_Handle(other.m_Handle)
    {
        other.m_Device = nullptr;
        other.m_Handle = RHIBufferHandle::Invalid;
    }

    ScopedBuffer& operator=(ScopedBuffer&& other) noexcept {
        if (this != &other) {
            Reset();
            m_Device = other.m_Device;
            m_Handle = other.m_Handle;
            other.m_Device = nullptr;
            other.m_Handle = RHIBufferHandle::Invalid;
        }
        return *this;
    }

    ScopedBuffer(const ScopedBuffer&) = delete;
    ScopedBuffer& operator=(const ScopedBuffer&) = delete;

    [[nodiscard]] RHIBufferHandle Get() const noexcept { return m_Handle; }
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_Handle != RHIBufferHandle::Invalid;
    }

    void Reset() {
        if (m_Device && m_Handle != RHIBufferHandle::Invalid) {
            (void)m_Device->DestroyBuffer(m_Handle);
        }
        m_Device = nullptr;
        m_Handle = RHIBufferHandle::Invalid;
    }

    [[nodiscard]] RHIBufferHandle Release() noexcept {
        const auto h = m_Handle;
        m_Device = nullptr;
        m_Handle = RHIBufferHandle::Invalid;
        return h;
    }

private:
    IRHIDevice* m_Device = nullptr;
    RHIBufferHandle m_Handle = RHIBufferHandle::Invalid;
};

class ScopedTexture {
public:
    ScopedTexture() = default;
    ScopedTexture(IRHIDevice* device, RHITextureHandle handle) noexcept
        : m_Device(device)
        , m_Handle(handle)
    {
    }

    ~ScopedTexture() { Reset(); }

    ScopedTexture(ScopedTexture&& other) noexcept
        : m_Device(other.m_Device)
        , m_Handle(other.m_Handle)
    {
        other.m_Device = nullptr;
        other.m_Handle = RHITextureHandle::Invalid;
    }

    ScopedTexture& operator=(ScopedTexture&& other) noexcept {
        if (this != &other) {
            Reset();
            m_Device = other.m_Device;
            m_Handle = other.m_Handle;
            other.m_Device = nullptr;
            other.m_Handle = RHITextureHandle::Invalid;
        }
        return *this;
    }

    ScopedTexture(const ScopedTexture&) = delete;
    ScopedTexture& operator=(const ScopedTexture&) = delete;

    [[nodiscard]] RHITextureHandle Get() const noexcept { return m_Handle; }
    [[nodiscard]] explicit operator bool() const noexcept {
        return m_Handle != RHITextureHandle::Invalid;
    }

    void Reset() {
        if (m_Device && m_Handle != RHITextureHandle::Invalid) {
            (void)m_Device->DestroyTexture(m_Handle);
        }
        m_Device = nullptr;
        m_Handle = RHITextureHandle::Invalid;
    }

    [[nodiscard]] RHITextureHandle Release() noexcept {
        const auto h = m_Handle;
        m_Device = nullptr;
        m_Handle = RHITextureHandle::Invalid;
        return h;
    }

private:
    IRHIDevice* m_Device = nullptr;
    RHITextureHandle m_Handle = RHITextureHandle::Invalid;
};

} // namespace we::rhi
