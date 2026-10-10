#pragma once

#include <CoreMinimal.h>
#include <d3d12.h>
#include <RHI/IRRHIResource.h>

namespace PigeonEngine
{

	/*
	* D3D12-backend intermediate interfaces.
	*
	* Mirror the pattern in D3D11Native.h: sit between the back-end-agnostic
	* IRRHIxxx interfaces and the concrete D3D12 resource types so command
	* lists can pull native handles without dynamic_cast.
	*/

	class IRD3D12Texture : public IRRHITexture
	{
	public:
		virtual ID3D12Resource*					GetD3D12Resource() const noexcept		= 0;
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12SRVHandle() const noexcept { return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12RTVHandle() const noexcept { return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12DSVHandle() const noexcept { return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12UAVHandle() const noexcept { return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
		virtual D3D12_RESOURCE_STATES			GetD3D12DefaultState() const noexcept { return D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON; }
	};

	class IRD3D12Buffer : public IRRHIBuffer
	{
	public:
		virtual ID3D12Resource*					GetD3D12Resource() const noexcept		= 0;
		virtual D3D12_GPU_VIRTUAL_ADDRESS		GetD3D12GPUVirtualAddress() const noexcept { return 0; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12SRVHandle() const noexcept { return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12UAVHandle() const noexcept { return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 }; }
	};

	class IRD3D12Shader : public IRRHIShader
	{
	public:
		virtual const void*		GetByteCode() const noexcept			= 0;
		virtual UINT64			GetByteCodeSize() const noexcept		= 0;
	};

	class IRD3D12PipelineState : public IRRHIPipelineState
	{
	public:
		virtual ID3D12PipelineState*			GetD3D12PipelineState() const noexcept			= 0;
		virtual ID3D12RootSignature*			GetD3D12RootSignature() const noexcept			= 0;
		virtual D3D12_PRIMITIVE_TOPOLOGY		GetD3D12PrimitiveTopology() const noexcept		= 0;
	};

	class IRD3D12Sampler : public IRRHISampler
	{
	public:
		// D3D12 uses descriptor-heap-based samplers; the CPU handle is
		// sufficient at bind time.
		virtual D3D12_CPU_DESCRIPTOR_HANDLE		GetD3D12SamplerHandle() const noexcept		= 0;
	};

};	// PigeonEngine
