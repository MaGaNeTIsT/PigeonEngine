#pragma once

#include <CoreMinimal.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

	/*
	* Back-end root signature handle.
	*
	* The rendering layer builds a RRHIRootSignatureDesc for a shader program's
	* reflected parameter space and asks IRRHIDevice::CreateRootSignature for the
	* matching object; PSOs are then created against that handle and carry it (plus
	* the RRHIPipelineLayoutInfo that came with it) so the command list can bind
	* root parameters without ever touching reflection.
	*
	* Ownership: the device owns the object and returns the same handle for an equal
	* desc (the layout hash is the identity), so callers never destroy a signature
	* they did not create. IRRHIDevice::DestroyRootSignature exists for the device
	* side and for a desc that has to be dropped early; it must not be used to drop
	* a shared signature that live PSOs still reference.
	*
	* The interface is deliberately narrow: it is a back-end-agnostic identity, not
	* a data sink. D3D11 has no root signature concept at all, so its object carries
	* only the hash and the layout semantics stay "root index == constant buffer
	* slot" (see RRHIPipelineLayoutInfo).
	*/
	class IRRHIRootSignature
	{
	public:
		IRRHIRootSignature() = default;
		virtual ~IRRHIRootSignature() = default;

		// Hash of the layout this object was created from. Equal hashes mean
		// identical root parameter layouts, which is what the signature and PSO
		// caches key on.
		virtual UINT64		GetLayoutHash() const noexcept = 0;

		// Binding layout of this signature, derived by the back-end from the same root
		// parameter list it built the signature from. PSOs copy it, and the render layer
		// reads it to know which root carries what; exposing it here is what keeps that
		// answer single sourced instead of being derived once per layer.
		virtual const RRHIPipelineLayoutInfo&	GetLayoutInfo() const noexcept = 0;

	public:
		IRRHIRootSignature(const IRRHIRootSignature&) = delete;
		IRRHIRootSignature(IRRHIRootSignature&&) = delete;
		IRRHIRootSignature& operator=(const IRRHIRootSignature&) = delete;
		IRRHIRootSignature& operator=(IRRHIRootSignature&&) = delete;
	};

	/*
	* Canonical layout hash of a root signature desc: the identity every cache keys on
	* (signature cache, PSO cache, LRU of layout objects). Parameter order is part of it,
	* because the order decides which root index carries what, so the same parameters in
	* another order are another signature.
	*
	* The render layer's layout generator writes this value into
	* RRHIRootSignatureDesc::LayoutHash; a back-end that receives a desc without one falls
	* back to this function, so a layout's identity is defined in exactly one place.
	*/
	PE_NODISCARD static inline UINT64 HashRHIRootSignatureDesc(const RRHIRootSignatureDesc& InDesc) noexcept
	{
		UINT64 Hash = 0ull;
		auto HashCombine = [&Hash](UINT64 InValue)
		{
			Hash ^= InValue + 0x9e3779b97f4a7c15ull + (Hash << 6) + (Hash >> 2);
		};

		HashCombine(static_cast<UINT64>(InDesc.bAllowInputAssembler));
		HashCombine(static_cast<UINT64>(InDesc.bHeapDirectlyIndexed));

		const UINT32 ParameterCount = InDesc.Parameters.Num<UINT32>();
		HashCombine(static_cast<UINT64>(ParameterCount));
		for (UINT32 i = 0u; i < ParameterCount; i++)
		{
			const RRHIRootParameterDesc& Parameter = InDesc.Parameters[i];
			HashCombine(static_cast<UINT64>(Parameter.Type));
			HashCombine(static_cast<UINT64>(Parameter.Visibility));
			HashCombine(static_cast<UINT64>(Parameter.ShaderRegister));
			HashCombine(static_cast<UINT64>(Parameter.RegisterSpace));
			HashCombine(static_cast<UINT64>(Parameter.Num32BitValues));
			HashCombine(static_cast<UINT64>(Parameter.Range.Type));
			HashCombine(static_cast<UINT64>(Parameter.Range.BaseShaderRegister));
			HashCombine(static_cast<UINT64>(Parameter.Range.NumDescriptors));
			HashCombine(static_cast<UINT64>(Parameter.Range.RegisterSpace));
		}
		return Hash;
	}

};	// PigeonEngine
