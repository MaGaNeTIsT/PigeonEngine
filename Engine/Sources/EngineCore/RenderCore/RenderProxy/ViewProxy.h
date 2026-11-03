#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <Renderer/ViewPrimitiveList.h>
#include <Renderer/RenderFilter.h>
#include <Shader/ShaderEngineParameters.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>

namespace PigeonEngine
{

	class RScene;
	class EShaderProgram;
	class IRRHIDevice;
	class IRCommandList;

	/*
	* The view parameters are published into the ViewParams block the shader source declares,
	* by name, and bound at the root the reading program's binding plan reports. The block is
	* shared by every program that declares it, so one instance of this proxy publishes it for
	* the whole frame rather than per material.
	*/
	class RViewProxy : public RBaseSceneProxy
	{
	public:
		RViewProxy(const PCameraComponent* InComponent);
		void	SetupProxy(const BOOL32 InIsMainView, const ERenderViewMatrices& InMatrices, const ERenderViewParams& InParams);
	public:
		BOOL32						IsMainRenderView() const;
		// The primitives this view decided to collect. The collection fills it and every pass
		// that consumes the view reads it; a view without a list draws nothing.
		RViewPrimitiveList&			GetPrimitiveList();
		const RViewPrimitiveList&	GetPrimitiveList() const;
		// Filter inputs. The kind decides the default collection rules; the masks are read by
		// the central registry and by the passes.
		PE_NODISCARD EViewKindType	GetViewKind() const { return ViewKind; }
		PE_NODISCARD UINT64			GetPrimitiveCategoryMask() const { return PrimitiveCategoryMask; }
		PE_NODISCARD UINT64			GetPassMask() const { return PassMask; }
		PE_NODISCARD UINT64			GetFilterFlags() const { return FilterFlags; }
		void						SetPrimitiveCategoryMask(const UINT64 InMask) { PrimitiveCategoryMask = InMask; }
		void						SetPassMask(const UINT64 InMask) { PassMask = InMask; }
		void						SetFilterFlags(const UINT64 InFlags) { FilterFlags = InFlags; }
		// The view's own collection escape hatch. The default answers TRUE; a view with a
		// rule that is not expressible as a category mask overrides it.
		virtual BOOL32				ShouldIncludePrimitive(const RPrimitiveSceneProxy& InPrimitive) const;
		// Which passes this view participates in. Bit N is ERenderPassType value N; a mask of
		// zero means every pass.
		PE_NODISCARD BOOL32			IsPassVisible(const FRenderPassDesc& InPassDesc) const;
		ECameraViewInfo&			GetViewInfo();
		const ECameraViewInfo&		GetViewInfo() const;
		EViewport&					GetRenderViewport();
		const EViewport&			GetRenderViewport() const;
		EViewMatrix&				GetViewMatrix();
		const EViewMatrix&			GetViewMatrix() const;
		EFrustum&					GetViewFrustum();
		const EFrustum&				GetViewFrustum() const;
	public:
		// Drops every primitive the previous frame collected. The collection appends, so a
		// reset before it is what keeps a primitive that stopped being visible from being
		// collected again.
		void	ResetPrimitiveList();
		void	UpdateViewSettings(const BOOL32 InIsMainView);
		void	UpdateMatrices(const ERenderViewMatrices& InMatrices);
		void	UpdateViewParams(const ERenderViewParams& InParams);
		// Builds the ViewParams mirrors and buffers from the first program that declares the
		// block. FALSE while no such program was seen: the view is simply not published in
		// that frame, and the next program gets to supply the layout.
		BOOL32	EnsureParameterBlock(IRRHIDevice* InRenderDevice, const EShaderProgram& InProgram);
		// Writes the values and pushes them into the gpu buffers. Early outs while nothing
		// moved, so a still camera costs one flag test per frame.
		void	UpdateRenderResource();
		// Binds the view block at the root the program's plan reports. Must run after the
		// pipeline state is set: switching a root signature voids every root binding.
		void	BindParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram) const;
	protected:
		// Named at construction: the name is the block's identity, so there is no unnamed state
		// for a later edit to leave it in (an unnamed block never initializes, uploads or binds).
		RShaderParameterBlock		ViewParameterBlock{ EName(SHADER_BLOCK_VIEW_PARAMS) };
		// TRUE while a value the block carries has moved since the last publish.
		BOOL32						bParameterValuesDirty;
		BOOL32						IsMainView;
		EViewKindType				ViewKind = EViewKindType::VIEW_KIND_CAMERA;
		UINT64						PrimitiveCategoryMask = 0ull;
		UINT64						PassMask = 0ull;
		UINT64						FilterFlags = 0ull;
		RViewPrimitiveList			PrimitiveList;
	protected:
		// A subclass that installs its own domain (a shadow view) sets the kind before the
		// first collection.
		void	SetViewKind(const EViewKindType InKind) { ViewKind = InKind; }
	protected:
		// A subclass that installs its own domain - a shadow view, whose projection comes from a
		// light - writes it directly and then calls this, which is what the next publish watches.
		// The camera path reaches the same flag through UpdateMatrices/UpdateViewParams.
		void	MarkViewParametersDirty() { bParameterValuesDirty = TRUE; }
		/*
		* Rebuilds the world space frustum from the view space one and the proxy's transform.
		*
		* GeneratePlaneWorldSpace transforms the corner points in place, so deriving the world
		* frustum from the value it produced last time would apply the camera transform again on
		* every move. The view space copy is kept for exactly this reason: a camera that only
		* moved reports matrices without new view params, and the world frustum still has to be
		* rebuilt from the same source rather than from its own previous output.
		*/
		void	RebuildWorldViewFrustum();
	protected:
		ECameraViewInfo				CameraViewInfo;
		// The frustum as the camera states it, in view space. ViewDomainInfo.ViewFrustum is the
		// world space copy derived from this one.
		EFrustum					LocalViewFrustum;
		EViewDomainInfo				ViewDomainInfo;
	protected:
		const PCameraComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RViewProxy)

	};

};
