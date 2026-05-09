#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#include <RHI/IRRHIResource.h>

namespace PigeonEngine
{

	/*
	* D3D11-backend intermediate interfaces.
	*
	* Sit between the back-end-agnostic IRRHIxxx interfaces (IRRHIBuffer,
	* IRRHITexture, ...) and the concrete D3D11 resource classes
	* (RBufferResource, RTexture2DResource, ...). They expose the native
	* ID3D11* objects via virtual getters so RCommandListD3D11 can pull
	* SRVs / RTVs / etc. through a single static_cast + virtual call,
	* without enumerating concrete types via dynamic_cast.
	*
	* All getters default to returning nullptr; concrete classes override
	* only the ones that make sense for their particular resource shape
	* (RTexture2DResource has SRV but no RTV/DSV/UAV, etc.). This keeps
	* per-class overrides minimal while letting the command list speak
	* in a single uniform interface.
	*
	* The D3D12 back-end will introduce a parallel set of IRD3D12xxx
	* mid-interfaces; engine-facing code never touches either, only the
	* corresponding command list does.
	*/

	class IRD3D11Texture : public IRRHITexture
	{
	public:
		virtual ID3D11Resource*				GetD3D11Resource()const = 0;
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const { return nullptr; }
		virtual ID3D11RenderTargetView*		GetD3D11RTV()const { return nullptr; }
		virtual ID3D11DepthStencilView*		GetD3D11DSV()const { return nullptr; }
		virtual ID3D11UnorderedAccessView*	GetD3D11UAV()const { return nullptr; }
	};

	class IRD3D11Buffer : public IRRHIBuffer
	{
	public:
		virtual ID3D11Buffer*				GetD3D11Buffer()const = 0;
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const { return nullptr; }
		virtual ID3D11UnorderedAccessView*	GetD3D11UAV()const { return nullptr; }
	};

	class IRD3D11Sampler : public IRRHISampler
	{
	public:
		virtual ID3D11SamplerState*			GetD3D11SamplerState()const = 0;
	};

	class IRD3D11Shader : public IRRHIShader
	{
	public:
		virtual ID3D11VertexShader*			GetD3D11VS()const { return nullptr; }
		virtual ID3D11PixelShader*			GetD3D11PS()const { return nullptr; }
		virtual ID3D11ComputeShader*		GetD3D11CS()const { return nullptr; }
		virtual ID3D11InputLayout*			GetD3D11InputLayout()const { return nullptr; }
	};

}
