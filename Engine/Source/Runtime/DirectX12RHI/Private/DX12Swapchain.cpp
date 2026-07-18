#include "DX12Device.h"
#include "DX12Internal.h"

#include "Core/LogCategory.h"
#include "Core/Logger.h"
#include "Platform/NativeHandle.h"
#include "RHI/ShaderBytecode.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

namespace we::rhi::dx12 {
DX12Swapchain::DX12Swapchain(DX12Device* device)
    : m_Device(device)
{
}

DX12Swapchain::~DX12Swapchain() {
    Destroy();
}

RHIResult<void> DX12Swapchain::Create(const DeviceDesc& desc) {
    if (!m_Device || !m_Device->GetD3DDevice()) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Device not ready.", "Swapchain::Create");
    }
    if (desc.window.type != we::platform::NativeWindowType::Win32Hwnd || !desc.window.window) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Expected Win32Hwnd for DXGI swapchain.", "Swapchain::Create");
    }

    const HWND hwnd = static_cast<HWND>(desc.window.window);
    RECT client{};
    GetClientRect(hwnd, &client);
    m_Extent.width = client.right > client.left ? static_cast<uint32_t>(client.right - client.left) : 1280u;
    m_Extent.height = client.bottom > client.top ? static_cast<uint32_t>(client.bottom - client.top) : 720u;
    if (m_Extent.width == 0) {
        m_Extent.width = 1;
    }
    if (m_Extent.height == 0) {
        m_Extent.height = 1;
    }
    m_Format = Format::B8G8R8A8_UNORM;

    // Prefer the device factory; fall back to a local one if needed.
    ComPtr<IDXGIFactory4> factory = m_Device->GetFactory();
    if (!factory) {
        UINT flags = 0;
        if (desc.enableValidation) {
            flags |= DXGI_CREATE_FACTORY_DEBUG;
        }
        if (FAILED(CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory)))) {
            if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)))) {
                return RHIError::Make(RHIErrorCode::BackendFailure, "CreateDXGIFactory2 failed.", "Swapchain::Create");
            }
        }
    }

    DXGI_SWAP_CHAIN_DESC1 scDesc{};
    scDesc.Width = m_Extent.width;
    scDesc.Height = m_Extent.height;
    scDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

    ComPtr<IDXGISwapChain1> swap1;
    auto& q = m_Device->GetQueue(QueueType::Graphics);
    ID3D12CommandQueue* queue = static_cast<DX12Queue&>(q).Get();
    if (!queue) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Graphics queue is null.", "Swapchain::Create");
    }

    const HRESULT hr = factory->CreateSwapChainForHwnd(
        queue,
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        &swap1);
    if (FAILED(hr) || !swap1) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateSwapChainForHwnd failed.", "Swapchain::Create", hr);
    }
    factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

    if (FAILED(swap1.As(&m_Swap)) || !m_Swap) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "QueryInterface IDXGISwapChain3 failed.", "Swapchain::Create");
    }

    m_Handles.clear();
    m_Handles.resize(scDesc.BufferCount);
    for (UINT i = 0; i < scDesc.BufferCount; ++i) {
        ComPtr<ID3D12Resource> buffer;
        if (FAILED(m_Swap->GetBuffer(i, IID_PPV_ARGS(&buffer)))) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "GetBuffer failed.", "Swapchain::Create");
        }
        m_Handles[i] = m_Device->RegisterSwapchainTexture(buffer, m_Extent, m_Format);
    }
    m_Index = m_Swap->GetCurrentBackBufferIndex();
    return RHIResult<void>::Success();
}

void DX12Swapchain::Destroy() {
    if (m_Device && !m_Handles.empty()) {
        m_Device->ClearSwapchainTextures(m_Handles);
    }
    m_Handles.clear();
    m_Swap.Reset();
}

RHITextureHandle DX12Swapchain::GetCurrentImage() const {
    if (m_Handles.empty()) {
        return RHITextureHandle::Invalid;
    }
    return m_Handles[m_Index % m_Handles.size()];
}

RHIResult<void> DX12Swapchain::Resize(Extent2D extent) {
    if (!m_Swap || !m_Device) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not created.", "Resize");
    }
    m_Device->WaitIdle();
    m_Device->ClearSwapchainTextures(m_Handles);
    m_Handles.clear();

    m_Extent.width = extent.width ? extent.width : 1;
    m_Extent.height = extent.height ? extent.height : 1;

    const HRESULT hr = m_Swap->ResizeBuffers(2, m_Extent.width, m_Extent.height, DXGI_FORMAT_B8G8R8A8_UNORM, 0);
    if (FAILED(hr)) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "ResizeBuffers failed.", "Resize", hr);
    }

    m_Handles.resize(2);
    for (UINT i = 0; i < 2; ++i) {
        ComPtr<ID3D12Resource> buffer;
        if (FAILED(m_Swap->GetBuffer(i, IID_PPV_ARGS(&buffer)))) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "GetBuffer failed after resize.", "Resize");
        }
        m_Handles[i] = m_Device->RegisterSwapchainTexture(buffer, m_Extent, m_Format);
    }
    m_Index = m_Swap->GetCurrentBackBufferIndex();
    return RHIResult<void>::Success();
}

RHIResult<uint32_t> DX12Swapchain::AcquireNextImage() {
    if (!m_Swap) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not created.", "AcquireNextImage");
    }
    m_Index = m_Swap->GetCurrentBackBufferIndex();
    return m_Index;
}

RHIResult<void> DX12Swapchain::Present() {
    if (!m_Swap) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Swapchain not created.", "Present");
    }
    const HRESULT hr = m_Swap->Present(1, 0);
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        HRESULT reason = DXGI_ERROR_DEVICE_REMOVED;
        if (m_Device && m_Device->GetD3DDevice()) {
            reason = m_Device->GetD3DDevice()->GetDeviceRemovedReason();
        }
        return RHIError::Make(RHIErrorCode::DeviceLost, "DX12 device removed/reset during Present.", "Present", reason);
    }
    if (hr == DXGI_STATUS_OCCLUDED) {
        return RHIResult<void>::Success();
    }
    if (FAILED(hr)) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Present failed.", "Present", hr);
    }
    return RHIResult<void>::Success();
}
} // namespace we::rhi::dx12

#endif