#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include "ViewProxy.h"

namespace PigeonEngine
{

	class RBaseSceneProxy;

	/*
	* Why a shadow view exists.
	*
	* The kind is the only place the renderer would need to know what it is drawing, and it does
	* not read it: every kind goes through the same depth pass. It is kept for diagnostics (a GPU
	* event name, a log line naming the light) and for the slot builder, which needs to know
	* whether a request is one of several cascades or one of six point faces.
	*/
	enum EShadowViewKindType : UINT8
	{
		SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE	= 0,
		SHADOW_VIEW_KIND_SPOT					= 1,
		SHADOW_VIEW_KIND_POINT_FACE				= 2,
		SHADOW_VIEW_KIND_COUNT
	};

	/*
	* What a light asks for, one request per view.
	*
	* The light fills the parts it knows - which view, how many of its own views exist, where the
	* cascade ends, how large a tile it wants and how important it is - and the renderer decides
	* the final tile size and where that tile lands in the atlas.
	*/
	struct FShadowViewRequest
	{
		EShadowViewKindType	Kind			= EShadowViewKindType::SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE;
		// Cascade index, or point face index. Always below Count.
		UINT32				Index			= 0u;
		// How many views this light produces in total. The renderer keeps one light's views
		// contiguous in the slot table, which is what lets the shader find them from one address.
		UINT32				Count			= 1u;
		// World space distance from the view origin that ends this cascade. The receiver compares
		// a pixel's depth against it to pick a cascade. FLT_MAX for a view that is not a cascade.
		FLOAT				SplitDistance	= PE_FLOAT32_MAX;
		// Tile edge this light would like. The renderer clamps it against the type cap and the
		// budget and may hand back something smaller.
		UINT32				RequestedSize	= 1024u;
		// Screen coverage the light estimates for itself; larger is more important. Used only to
		// order local light allocation, and only when the budget cannot serve every candidate.
		FLOAT				Importance		= 1.f;
	};

	/*
	* One element of the GPU slot table, indexed by the shadow map index the light list carries.
	*
	* Layout rules are the light lists' rules: one 16 byte slot per member, order is the layout,
	* and the count side (C++ struct size vs reflected field count) is checked at runtime by the
	* renderer. The matrix is stored as four rows because a single field cannot legally carry more
	* than sixteen scalars through the parameter layer; four float4 rows are also what a
	* StructuredBuffer element reads most cheaply.
	*
	*   ViewProjectionRow0..3  world -> clip, row major, applied as mul(Vector, Matrix)
	*   AtlasTile              xy = tile UV scale, zw = tile UV offset in the atlas
	*   ShadowParams           x = this cascade's far split (world)
	*                          y = 1 / tile size (texel size in UV)
	*                          z = cascade or face index
	*                          w = 1 when the slot is valid
	*   BiasParams             x = constant depth bias, y = slope scaled bias,
	*                          z = normal offset in world units, w = cascade blend fraction
	*/
	struct EShadowSlotGPU
	{
		Vector4	ViewProjectionRow0;
		Vector4	ViewProjectionRow1;
		Vector4	ViewProjectionRow2;
		Vector4	ViewProjectionRow3;
		Vector4	AtlasTile;
		Vector4	ShadowParams;
		Vector4	BiasParams;
	};
	static_assert(sizeof(EShadowSlotGPU) == 112u, "EShadowSlotGPU must be seven 16 byte slots");

	/*
	* A shadow view: a view whose projection comes from a light rather than a camera.
	*
	* It is a plain RViewProxy so every existing path applies unchanged - ViewParams publishing
	* and binding, the per view visibility map, and the per primitive draw that reads the current
	* view. What it adds is where the view came from, which the renderer needs to rebuild it and
	* which the atlas needs to know where it writes.
	*/
	class RShadowViewProxy final : public RViewProxy
	{
	public:
		/*
		* Installs the domain a light built. The anchor and rotation are the light's view position
		* and orientation, passed separately rather than decomposed from the matrix: the ViewParams
		* publish needs the world position and the forward axis, and decomposing a matrix to get
		* them back would lose precision for no reason.
		*/
		void	SetupShadowView(const EShadowViewKindType InKind, const RBaseSceneProxy* InLightProxy
			, const UINT32 InShadowSlot, const UINT32 InCascadeIndex, const UINT32 InCascadeCount
			, const EViewDomainInfo& InDomainInfo
			, const Vector3& InViewAnchor, const Quaternion& InViewRotation);
		// Renderer owned identity: one view per (light, kind, index) per camera view, so a frame
		// rebuild can find the proxy it built last frame instead of allocating a new one (and with
		// it a new ViewParams block) every frame.
		void	SetShadowViewKey(const UINT64 InKey) { ShadowViewKey = InKey; }
		PE_NODISCARD UINT64 GetShadowViewKey()const { return ShadowViewKey; }
		// Marks the view as in use this frame. The renderer clears it before rebuilding and
		// deletes whatever stayed clear, which is what keeps a removed light from leaving a view
		// behind.
		void	MarkUsed()const { bUsedThisFrame = TRUE; }
		void	ClearUsed()const { bUsedThisFrame = FALSE; }
		PE_NODISCARD BOOL32 IsUsedThisFrame()const { return bUsedThisFrame; }
	public:
		PE_NODISCARD EShadowViewKindType	GetShadowKind()const { return Kind; }
		PE_NODISCARD const RBaseSceneProxy*	GetLightProxy()const { return LightProxy; }
		PE_NODISCARD UINT32					GetShadowSlot()const { return ShadowSlot; }
		PE_NODISCARD UINT32					GetCascadeIndex()const { return CascadeIndex; }
		PE_NODISCARD UINT32					GetCascadeCount()const { return CascadeCount; }
		PE_NODISCARD const EViewDomainInfo&	GetShadowDomainInfo()const { return ShadowDomainInfo; }
		// Atlas tile this view renders into, as a pixel rectangle. The domain's viewport is set to
		// the same rectangle, so the two cannot disagree.
		PE_NODISCARD const EViewport&		GetAtlasTile()const { return ShadowDomainInfo.RenderViewport; }
	private:
		EShadowViewKindType		Kind			= EShadowViewKindType::SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE;
		const RBaseSceneProxy*	LightProxy		= nullptr;
		UINT32					ShadowSlot		= 0u;
		UINT32					CascadeIndex	= 0u;
		UINT32					CascadeCount	= 1u;
		UINT64					ShadowViewKey	= 0ull;
		EViewDomainInfo			ShadowDomainInfo;
		mutable BOOL32			bUsedThisFrame	= FALSE;

		RENDER_PROXY_CLASS_BODY(RShadowViewProxy)

	};

} // namespace PigeonEngine
