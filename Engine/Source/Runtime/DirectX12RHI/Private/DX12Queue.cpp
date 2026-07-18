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
RHIResult<void> DX12Queue::Submit(IRHICommandList& commandList) {
    if (!m_Queue) {
        return RHIError::Make(RHIErrorCode::NotInitialized, "Queue is null.", "Submit");
    }
    auto* dxCmd = static_cast<DX12CommandList*>(&commandList);
    ID3D12GraphicsCommandList* list = dxCmd->Get();
    if (!list) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Command list is null.", "Submit");
    }
    commandList.End();
    const HRESULT hr = list->Close();
    if (FAILED(hr)) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "ID3D12GraphicsCommandList::Close failed.", "Submit", hr);
    }
    ID3D12CommandList* lists[] = {list};
    m_Queue->ExecuteCommandLists(1, lists);
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Queue::Submit(const SubmitDesc& desc) {
    if (!desc.commandList) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "SubmitDesc has no command list.", "Submit");
    }
    return Submit(*desc.commandList);
}

RHIResult<void> DX12Queue::WaitIdle() {
    if (!m_Queue) {
        return RHIResult<void>::Success();
    }
    ComPtr<ID3D12Device> device;
    if (FAILED(m_Queue->GetDevice(IID_PPV_ARGS(&device))) || !device) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Failed to get device from queue.", "WaitIdle");
    }
    ComPtr<ID3D12Fence> fence;
    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "CreateFence failed.", "WaitIdle");
    }
    const UINT64 value = 1;
    if (FAILED(m_Queue->Signal(fence.Get(), value))) {
        return RHIError::Make(RHIErrorCode::BackendFailure, "Queue Signal failed.", "WaitIdle");
    }
    if (fence->GetCompletedValue() < value) {
        HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!event) {
            return RHIError::Make(RHIErrorCode::BackendFailure, "CreateEvent failed.", "WaitIdle");
        }
        fence->SetEventOnCompletion(value, event);
        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
    return RHIResult<void>::Success();
}
} // namespace we::rhi::dx12

#endif