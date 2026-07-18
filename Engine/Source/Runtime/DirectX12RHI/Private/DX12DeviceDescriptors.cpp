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

RHIResult<RHIDescriptorSetLayoutHandle> DX12Device::CreateDescriptorSetLayout(const DescriptorSetLayoutDesc& desc) {
    const auto handle = static_cast<RHIDescriptorSetLayoutHandle>(AllocHandle());
    DX12DescriptorSetLayout layout{};
    layout.desc = desc;
    layout.bindings.reserve(desc.bindings.size());
    uint32_t srvCursor = 0;
    uint32_t samplerCursor = 0;
    for (const auto& b : desc.bindings) {
        const uint32_t count = b.count ? b.count : 1u;
        DX12DescriptorBinding binding{};
        binding.binding = b;
        if (BindingUsesSrvHeap(b.type)) {
            binding.usesSrvUavCbv = true;
            binding.srvUavCbvOffset = srvCursor;
            srvCursor += count;
        }
        if (BindingUsesSamplerHeap(b.type)) {
            binding.usesSampler = true;
            binding.samplerOffset = samplerCursor;
            samplerCursor += count;
        }
        layout.bindings.push_back(binding);
    }
    layout.srvUavCbvCount = srvCursor;
    layout.samplerCount = samplerCursor;
    m_SetLayouts.emplace(static_cast<uint64_t>(handle), std::move(layout));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyDescriptorSetLayout(RHIDescriptorSetLayoutHandle handle) {
    if (m_SetLayouts.find(static_cast<uint64_t>(handle)) == m_SetLayouts.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor set layout.", "DestroyDescriptorSetLayout");
    }
    EnqueueDeferred(DeferredKind::DescriptorSetLayout, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIDescriptorPoolHandle> DX12Device::CreateDescriptorPool(const DescriptorPoolDesc& desc) {
    const auto handle = static_cast<RHIDescriptorPoolHandle>(AllocHandle());
    DX12DescriptorPool pool{};
    pool.desc = desc;
    m_Pools.emplace(static_cast<uint64_t>(handle), std::move(pool));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyDescriptorPool(RHIDescriptorPoolHandle handle) {
    if (m_Pools.find(static_cast<uint64_t>(handle)) == m_Pools.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "DestroyDescriptorPool");
    }
    EnqueueDeferred(DeferredKind::DescriptorPool, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<void> DX12Device::ResetDescriptorPool(RHIDescriptorPoolHandle handle) {
    if (!m_Pools.count(static_cast<uint64_t>(handle))) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown descriptor pool.", "ResetDescriptorPool");
    }
    for (auto it = m_Sets.begin(); it != m_Sets.end();) {
        if (it->second.pool == handle) {
            it = m_Sets.erase(it);
        } else {
            ++it;
        }
    }
    return RHIResult<void>::Success();
}

RHIResult<RHIDescriptorSetHandle> DX12Device::AllocateDescriptorSet(const DescriptorSetAllocateDesc& desc) {
    auto* setLayout = FindDescriptorSetLayout(desc.layout);
    if (!m_Pools.count(static_cast<uint64_t>(desc.pool)) || !setLayout) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Invalid pool or layout.", "AllocateDescriptorSet");
    }
    uint32_t srvOffset = 0;
    uint32_t samplerOffset = 0;
    if (!AllocateShaderVisibleSlots(
            setLayout->srvUavCbvCount, setLayout->samplerCount, srvOffset, samplerOffset)) {
        return RHIError::Make(RHIErrorCode::OutOfMemory, "Descriptor heap exhausted.", "AllocateDescriptorSet");
    }
    const auto handle = static_cast<RHIDescriptorSetHandle>(AllocHandle());
    DX12DescriptorSet set{};
    set.pool = desc.pool;
    set.layout = desc.layout;
    set.srvUavCbvHeapOffset = srvOffset;
    set.samplerHeapOffset = samplerOffset;
    set.srvUavCbvCount = setLayout->srvUavCbvCount;
    set.samplerCount = setLayout->samplerCount;
    m_Sets.emplace(static_cast<uint64_t>(handle), std::move(set));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

void DX12Device::UpdateDescriptorSets(std::span<const WriteDescriptorSet> writes) {
    for (const auto& write : writes) {
        auto* set = FindDescriptorSet(write.set);
        auto* layout = set ? FindDescriptorSetLayout(set->layout) : nullptr;
        if (!set || !layout || write.count == 0) {
            continue;
        }

        const DX12DescriptorBinding* bindingMeta = nullptr;
        for (const auto& b : layout->bindings) {
            if (b.binding.binding == write.binding) {
                bindingMeta = &b;
                break;
            }
        }
        if (!bindingMeta) {
            continue;
        }

        for (uint32_t i = 0; i < write.count; ++i) {
            if (write.bufferInfos
                && (write.type == DescriptorType::UniformBuffer
                    || write.type == DescriptorType::StorageBuffer)) {
                const auto& bi = write.bufferInfos[i];
                auto* buf = FindBuffer(bi.buffer);
                if (!buf || !buf->resource) {
                    continue;
                }
                const uint32_t slot = set->srvUavCbvHeapOffset
                    + bindingMeta->srvUavCbvOffset
                    + write.arrayElement
                    + i;
                if (write.type == DescriptorType::UniformBuffer) {
                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
                    cbv.BufferLocation = buf->resource->GetGPUVirtualAddress() + bi.offset;
                    const uint64_t rawSize = bi.range == ~0ull
                        ? (buf->desc.size > bi.offset ? buf->desc.size - bi.offset : 0)
                        : bi.range;
                    cbv.SizeInBytes = static_cast<UINT>((rawSize + 255ull) & ~255ull);
                    if (cbv.SizeInBytes == 0) {
                        continue;
                    }
                    m_Device->CreateConstantBufferView(&cbv, SrvCpu(slot));
                } else {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
                    uav.Format = DXGI_FORMAT_R32_TYPELESS;
                    uav.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    uav.Buffer.FirstElement = bi.offset / 4;
                    const uint64_t rawSize = bi.range == ~0ull
                        ? (buf->desc.size > bi.offset ? buf->desc.size - bi.offset : 0)
                        : bi.range;
                    uav.Buffer.NumElements = static_cast<UINT>(rawSize / 4);
                    uav.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
                    m_Device->CreateUnorderedAccessView(buf->resource.Get(), nullptr, &uav, SrvCpu(slot));
                }
            }

            if (write.imageInfos
                && (write.type == DescriptorType::SampledImage
                    || write.type == DescriptorType::CombinedImageSampler
                    || write.type == DescriptorType::StorageImage
                    || write.type == DescriptorType::Sampler)) {
                const auto& ii = write.imageInfos[i];
                if (write.type != DescriptorType::Sampler
                    && bindingMeta->usesSrvUavCbv) {
                    auto* view = FindTextureView(ii.view);
                    auto* tex = view ? FindTexture(view->desc.texture) : nullptr;
                    if (tex && tex->resource) {
                        const uint32_t slot = set->srvUavCbvHeapOffset
                            + bindingMeta->srvUavCbvOffset
                            + write.arrayElement
                            + i;
                        if (write.type == DescriptorType::StorageImage) {
                            D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
                            uav.Format = view->format;
                            uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                            uav.Texture2D.MipSlice = view->desc.baseMip;
                            m_Device->CreateUnorderedAccessView(
                                tex->resource.Get(), nullptr, &uav, SrvCpu(slot));
                        } else {
                            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
                            srv.Format = view->format;
                            srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                            srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                            srv.Texture2D.MostDetailedMip = view->desc.baseMip;
                            srv.Texture2D.MipLevels = view->desc.mipCount ? view->desc.mipCount : 1;
                            m_Device->CreateShaderResourceView(
                                tex->resource.Get(), &srv, SrvCpu(slot));
                        }
                    }
                }
                if ((write.type == DescriptorType::Sampler
                        || write.type == DescriptorType::CombinedImageSampler)
                    && bindingMeta->usesSampler) {
                    auto* sampler = FindSampler(ii.sampler);
                    if (sampler) {
                        const uint32_t slot = set->samplerHeapOffset
                            + bindingMeta->samplerOffset
                            + write.arrayElement
                            + i;
                        m_Device->CreateSampler(&sampler->samplerDesc, SamplerCpu(slot));
                    }
                }
            }
        }
    }
}

RHIResult<RHIPipelineLayoutHandle> DX12Device::CreatePipelineLayout(const PipelineLayoutDesc& desc) {
    std::vector<D3D12_ROOT_PARAMETER> rootParams;
    std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
    ranges.reserve(64);
    rootParams.reserve(16);

    DX12PipelineLayout layout{};
    layout.desc = desc;
    layout.setRoots.assign(desc.setLayouts.size(), {});

    uint32_t pushBytes = desc.pushConstantSize;
    if (!desc.pushConstantRanges.empty()) {
        uint32_t end = 0;
        for (const auto& r : desc.pushConstantRanges) {
            end = (std::max)(end, r.offset + r.size);
        }
        pushBytes = end;
    }
    if (pushBytes > 0) {
        D3D12_ROOT_PARAMETER pc{};
        pc.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        pc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        pc.Constants.ShaderRegister = 0;
        pc.Constants.RegisterSpace = 0;
        pc.Constants.Num32BitValues = (pushBytes + 3u) / 4u;
        layout.pushConstantRootParam = static_cast<uint32_t>(rootParams.size());
        layout.pushConstantDwords = pc.Constants.Num32BitValues;
        rootParams.push_back(pc);
    }

    for (size_t setIdx = 0; setIdx < desc.setLayouts.size(); ++setIdx) {
        auto* setLayout = FindDescriptorSetLayout(desc.setLayouts[setIdx]);
        if (!setLayout) {
            return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown set layout.", "CreatePipelineLayout");
        }
        auto& mapping = layout.setRoots[setIdx];

        std::vector<D3D12_DESCRIPTOR_RANGE> srvRanges;
        std::vector<D3D12_DESCRIPTOR_RANGE> samplerRanges;
        for (const auto& b : setLayout->bindings) {
            const uint32_t count = b.binding.count ? b.binding.count : 1u;
            if (b.usesSrvUavCbv) {
                D3D12_DESCRIPTOR_RANGE range{};
                range.RangeType = ToSrvHeapRangeType(b.binding.type);
                range.NumDescriptors = count;
                range.BaseShaderRegister = b.binding.binding;
                range.RegisterSpace = static_cast<UINT>(setIdx);
                range.OffsetInDescriptorsFromTableStart = b.srvUavCbvOffset;
                srvRanges.push_back(range);
            }
            if (b.usesSampler) {
                D3D12_DESCRIPTOR_RANGE range{};
                range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                range.NumDescriptors = count;
                range.BaseShaderRegister = b.binding.binding;
                range.RegisterSpace = static_cast<UINT>(setIdx);
                range.OffsetInDescriptorsFromTableStart = b.samplerOffset;
                samplerRanges.push_back(range);
            }
        }

        if (!srvRanges.empty()) {
            const size_t base = ranges.size();
            ranges.insert(ranges.end(), srvRanges.begin(), srvRanges.end());
            mapping.srvUavCbvRootParam = static_cast<uint32_t>(rootParams.size());
            D3D12_ROOT_PARAMETER param{};
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(srvRanges.size());
            param.DescriptorTable.pDescriptorRanges = ranges.data() + base;
            rootParams.push_back(param);
        }
        if (!samplerRanges.empty()) {
            const size_t base = ranges.size();
            ranges.insert(ranges.end(), samplerRanges.begin(), samplerRanges.end());
            mapping.samplerRootParam = static_cast<uint32_t>(rootParams.size());
            D3D12_ROOT_PARAMETER param{};
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(samplerRanges.size());
            param.DescriptorTable.pDescriptorRanges = ranges.data() + base;
            rootParams.push_back(param);
        }
    }

    // Re-point DescriptorTable range pointers after final ranges vector growth.
    {
        size_t rangeCursor = 0;
        size_t paramIdx = (layout.pushConstantRootParam != UINT32_MAX) ? 1u : 0u;
        for (size_t setIdx = 0; setIdx < layout.setRoots.size(); ++setIdx) {
            auto& mapping = layout.setRoots[setIdx];
            auto* setLayout = FindDescriptorSetLayout(desc.setLayouts[setIdx]);
            if (!setLayout) {
                continue;
            }
            uint32_t srvRangeCount = 0;
            uint32_t samplerRangeCount = 0;
            for (const auto& b : setLayout->bindings) {
                if (b.usesSrvUavCbv) {
                    ++srvRangeCount;
                }
                if (b.usesSampler) {
                    ++samplerRangeCount;
                }
            }
            if (mapping.srvUavCbvRootParam != UINT32_MAX && paramIdx < rootParams.size()) {
                rootParams[paramIdx].DescriptorTable.pDescriptorRanges = ranges.data() + rangeCursor;
                rootParams[paramIdx].DescriptorTable.NumDescriptorRanges = srvRangeCount;
                rangeCursor += srvRangeCount;
                ++paramIdx;
            }
            if (mapping.samplerRootParam != UINT32_MAX && paramIdx < rootParams.size()) {
                rootParams[paramIdx].DescriptorTable.pDescriptorRanges = ranges.data() + rangeCursor;
                rootParams[paramIdx].DescriptorTable.NumDescriptorRanges = samplerRangeCount;
                rangeCursor += samplerRangeCount;
                ++paramIdx;
            }
        }
    }

    D3D12_ROOT_SIGNATURE_DESC rsDesc{};
    rsDesc.NumParameters = static_cast<UINT>(rootParams.size());
    rsDesc.pParameters = rootParams.empty() ? nullptr : rootParams.data();
    rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    const HRESULT hrSerialize = D3D12SerializeRootSignature(
        &rsDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
    if (FAILED(hrSerialize)) {
        const char* msg = (error && error->GetBufferPointer())
            ? static_cast<const char*>(error->GetBufferPointer())
            : "D3D12SerializeRootSignature failed.";
        return RHIError::Make(RHIErrorCode::BackendFailure, msg, "CreatePipelineLayout", hrSerialize);
    }
    const HRESULT hrCreate = m_Device->CreateRootSignature(
        0,
        signature->GetBufferPointer(),
        signature->GetBufferSize(),
        IID_PPV_ARGS(&layout.rootSignature));
    if (FAILED(hrCreate)) {
        return RHIError::Make(
            RHIErrorCode::BackendFailure, "CreateRootSignature failed.", "CreatePipelineLayout", hrCreate);
    }

    const auto handle = static_cast<RHIPipelineLayoutHandle>(AllocHandle());
    m_Layouts.emplace(static_cast<uint64_t>(handle), std::move(layout));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyPipelineLayout(RHIPipelineLayoutHandle handle) {
    if (m_Layouts.find(static_cast<uint64_t>(handle)) == m_Layouts.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown pipeline layout.", "DestroyPipelineLayout");
    }
    EnqueueDeferred(DeferredKind::PipelineLayout, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIGraphicsPipelineHandle> DX12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc& desc) {
    DX12GraphicsPipeline pipeline{};
    pipeline.desc = desc;
    pipeline.layoutHandle = desc.layout;

    auto* layout = FindPipelineLayout(desc.layout);
    if (!layout || !layout->rootSignature) {
        return RHIError::Make(
            RHIErrorCode::InvalidHandle, "Missing pipeline layout / root signature.", "CreateGraphicsPipeline");
    }

    auto vsIt = m_Shaders.find(static_cast<uint64_t>(desc.vertexShader));
    auto fsIt = m_Shaders.find(static_cast<uint64_t>(desc.fragmentShader));
    const bool haveShaders = vsIt != m_Shaders.end() && fsIt != m_Shaders.end()
        && !vsIt->second.bytecode.empty() && !fsIt->second.bytecode.empty();
    if (!haveShaders || !m_Device) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Missing shaders.", "CreateGraphicsPipeline");
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> elements;
    elements.reserve(desc.vertexAttributes.size());
    for (const auto& attr : desc.vertexAttributes) {
        D3D12_INPUT_ELEMENT_DESC el{};
        el.SemanticName = attr.semanticName ? attr.semanticName : "TEXCOORD";
        el.SemanticIndex = attr.semanticIndex != ~0u ? attr.semanticIndex : attr.location;
        el.Format = ToDxgiFormat(attr.format);
        el.InputSlot = attr.binding;
        el.AlignedByteOffset = attr.offset;
        el.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        el.InstanceDataStepRate = 0;
        for (const auto& binding : desc.vertexBindings) {
            if (binding.binding == attr.binding && binding.perInstance) {
                el.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
                el.InstanceDataStepRate = 1;
                break;
            }
        }
        elements.push_back(el);
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = layout->rootSignature.Get();
    psoDesc.VS = {vsIt->second.bytecode.data(), vsIt->second.bytecode.size()};
    psoDesc.PS = {fsIt->second.bytecode.data(), fsIt->second.bytecode.size()};

    const std::vector<Format>& colorFormats = !desc.colorFormats.empty()
        ? desc.colorFormats
        : std::vector<Format>{desc.colorFormat};
    psoDesc.NumRenderTargets = static_cast<UINT>(std::min<size_t>(colorFormats.size(), 8));
    for (UINT i = 0; i < psoDesc.NumRenderTargets; ++i) {
        psoDesc.RTVFormats[i] = ToDxgiFormat(colorFormats[i]);
        const BlendStateDesc& blend = !desc.colorBlends.empty()
            ? desc.colorBlends[std::min<size_t>(i, desc.colorBlends.size() - 1)]
            : desc.blend;
        ApplyBlendTarget(psoDesc.BlendState.RenderTarget[i], blend);
    }

    psoDesc.SampleMask = desc.multisample.sampleMask;
    psoDesc.RasterizerState.FillMode = desc.rasterizer.fillMode == FillMode::Wireframe
        ? D3D12_FILL_MODE_WIREFRAME
        : D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = desc.cullMode == CullMode::None
        ? D3D12_CULL_MODE_NONE
        : (desc.cullMode == CullMode::Front ? D3D12_CULL_MODE_FRONT : D3D12_CULL_MODE_BACK);
    psoDesc.RasterizerState.FrontCounterClockwise = desc.rasterizer.frontCounterClockwise ? TRUE : FALSE;
    psoDesc.RasterizerState.DepthClipEnable = desc.rasterizer.depthClamp ? FALSE : TRUE;
    psoDesc.RasterizerState.DepthBias = desc.rasterizer.depthBiasEnable
        ? static_cast<INT>(desc.rasterizer.depthBiasConstant)
        : 0;
    psoDesc.RasterizerState.SlopeScaledDepthBias = desc.rasterizer.depthBiasEnable
        ? desc.rasterizer.depthBiasSlope
        : 0.0f;
    psoDesc.RasterizerState.DepthBiasClamp = desc.rasterizer.depthBiasEnable
        ? desc.rasterizer.depthBiasClamp
        : 0.0f;

    psoDesc.DepthStencilState.DepthEnable = desc.depthTest ? TRUE : FALSE;
    psoDesc.DepthStencilState.DepthWriteMask = desc.depthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = ToCompare(desc.depthCompare);
    psoDesc.DepthStencilState.StencilEnable = desc.depthStencil.stencilTest ? TRUE : FALSE;
    if (desc.depthStencil.stencilTest) {
        psoDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(desc.depthStencil.front.compareMask);
        psoDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(desc.depthStencil.front.writeMask);
        auto applyFace = [](D3D12_DEPTH_STENCILOP_DESC& face, const StencilOpStateDesc& src) {
            face.StencilFailOp = ToStencilOp(src.failOp);
            face.StencilDepthFailOp = ToStencilOp(src.depthFailOp);
            face.StencilPassOp = ToStencilOp(src.passOp);
            face.StencilFunc = ToCompare(src.compareOp);
        };
        applyFace(psoDesc.DepthStencilState.FrontFace, desc.depthStencil.front);
        applyFace(psoDesc.DepthStencilState.BackFace, desc.depthStencil.back);
    }

    psoDesc.InputLayout = {elements.data(), static_cast<UINT>(elements.size())};
    psoDesc.PrimitiveTopologyType = ToTopologyType(desc.topology);
    psoDesc.DSVFormat = desc.depthAttachment ? ToDxgiFormat(desc.depthFormat) : DXGI_FORMAT_UNKNOWN;
    psoDesc.SampleDesc.Count = desc.multisample.sampleCount ? desc.multisample.sampleCount : 1;
    psoDesc.SampleDesc.Quality = 0;

    ComPtr<ID3D12PipelineState> pso;
    const HRESULT hr = m_Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr)) {
        return RHIError::Make(
            RHIErrorCode::BackendFailure, "CreateGraphicsPipelineState failed.", "CreateGraphicsPipeline", hr);
    }
    pipeline.rootSignature = layout->rootSignature;
    pipeline.pso = std::move(pso);

    const auto handle = static_cast<RHIGraphicsPipelineHandle>(AllocHandle());
    m_GfxPipelines.emplace(static_cast<uint64_t>(handle), std::move(pipeline));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyGraphicsPipeline(RHIGraphicsPipelineHandle handle) {
    if (m_GfxPipelines.find(static_cast<uint64_t>(handle)) == m_GfxPipelines.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown graphics pipeline.", "DestroyGraphicsPipeline");
    }
    EnqueueDeferred(DeferredKind::GraphicsPipeline, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}

RHIResult<RHIComputePipelineHandle> DX12Device::CreateComputePipeline(const ComputePipelineDesc& desc) {
    DX12ComputePipeline pipeline{};
    pipeline.desc = desc;

    auto* layout = FindPipelineLayout(desc.layout);
    if (!layout || !layout->rootSignature) {
        return RHIError::Make(
            RHIErrorCode::InvalidHandle, "Missing pipeline layout / root signature.", "CreateComputePipeline");
    }

    auto csIt = m_Shaders.find(static_cast<uint64_t>(desc.computeShader));
    if (csIt == m_Shaders.end() || csIt->second.bytecode.empty() || !m_Device) {
        return RHIError::Make(RHIErrorCode::InvalidArgument, "Missing compute shader.", "CreateComputePipeline");
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = layout->rootSignature.Get();
    psoDesc.CS = {csIt->second.bytecode.data(), csIt->second.bytecode.size()};
    ComPtr<ID3D12PipelineState> pso;
    const HRESULT hr = m_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&pso));
    if (FAILED(hr)) {
        return RHIError::Make(
            RHIErrorCode::BackendFailure, "CreateComputePipelineState failed.", "CreateComputePipeline", hr);
    }
    pipeline.rootSignature = layout->rootSignature;
    pipeline.pso = std::move(pso);

    const auto handle = static_cast<RHIComputePipelineHandle>(AllocHandle());
    m_ComputePipelines.emplace(static_cast<uint64_t>(handle), std::move(pipeline));
    ++m_Diagnostics.resourcesCreated;
    return handle;
}

RHIResult<void> DX12Device::DestroyComputePipeline(RHIComputePipelineHandle handle) {
    if (m_ComputePipelines.find(static_cast<uint64_t>(handle)) == m_ComputePipelines.end()) {
        return RHIError::Make(RHIErrorCode::InvalidHandle, "Unknown compute pipeline.", "DestroyComputePipeline");
    }
    EnqueueDeferred(DeferredKind::ComputePipeline, static_cast<uint64_t>(handle));
    return RHIResult<void>::Success();
}
} // namespace we::rhi::dx12

#endif
