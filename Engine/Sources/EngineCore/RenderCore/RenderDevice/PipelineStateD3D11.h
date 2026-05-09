#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#include <RenderCommon.h>
#include <RHI/IRRHIResource.h>

namespace PigeonEngine
{

	/*
	* D3D11 has no monolithic PSO. RPipelineStateD3D11 is the engine-side
	* aggregate that bundles every state object the new RHI's
	* IRRHIDevice::CreateGraphicsPipelineState / CreateComputePipelineState
	* needs to produce, plus the per-bind values (blend factor, stencil ref,
	* primitive topology) that D3D11 expects at bind time.
	*
	* RCommandListD3D11::SetPipelineState walks these fields and issues the
	* matching VSSetShader / IASetInputLayout / RSSetState / OMSetBlendState
	* / OMSetDepthStencilState / IASetPrimitiveTopology calls in one shot.
	*/
	class RPipelineStateD3D11 final : public IRRHIPipelineState
	{
	public:
		RPipelineStateD3D11() = default;
		virtual ~RPipelineStateD3D11() = default;

		RPipelineStateD3D11(const RPipelineStateD3D11&) = delete;
		RPipelineStateD3D11& operator=(const RPipelineStateD3D11&) = delete;

		// IRRHIPipelineState
		virtual BOOL8 IsCompute()const override { return bIsCompute; }

		// Graphics state (only valid if bIsCompute == FALSE)
		Microsoft::WRL::ComPtr<ID3D11VertexShader>		VertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>		PixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>		InputLayout;
		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	RasterizerState;
		Microsoft::WRL::ComPtr<ID3D11BlendState>		BlendState;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	DepthStencilState;

		// Compute state (only valid if bIsCompute == TRUE)
		Microsoft::WRL::ComPtr<ID3D11ComputeShader>		ComputeShader;

		// Per-bind values supplied at pipeline creation; CommandList may
		// override BlendFactor / StencilRef per draw via SetBlendFactor /
		// SetStencilRef (those rebind the same state object with the new
		// scalar).
		RPrimitiveTopologyType	PrimitiveTopology	= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		Color4					DefaultBlendFactor	= Color4(0.f, 0.f, 0.f, 0.f);
		UINT32					SampleMask			= 0xffffffffu;
		UINT32					DefaultStencilRef	= 0u;
		BOOL8					bIsCompute			= FALSE;
	};

}
