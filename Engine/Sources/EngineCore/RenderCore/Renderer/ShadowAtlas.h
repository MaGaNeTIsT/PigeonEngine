#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderResource.h>
#include <RHI/IRRHIDevice.h>
#include <RenderConfig/ShadowConfig.h>
#include <RenderProxy/ShadowViewProxy.h>

namespace PigeonEngine
{

	/*
	* One shelf allocated rectangle of the shadow atlas.
	*
	* Rect is the pixel rectangle the tile covers. A shadow view renders into it with a viewport
	* and scissor rather than into its own texture, which is what lets tiles of different sizes
	* share one depth target.
	*/
	struct FShadowTile
	{
		EViewport	Rect;
		UINT32		Size	= 0u;
		BOOL32		bUsed	= FALSE;
	};

	/*
	* The depth atlas of one camera view.
	*
	* It is a single depth texture plus a shelf allocator. The allocator is fed in descending size
	* order by the renderer, so the large cascades land first and the small local light tiles fill
	* the remaining width; a tile that does not fit the current row starts a new one, and a tile
	* that does not fit anything is refused, which is the renderer's cue to downgrade or drop.
	*
	* Why not a Texture2DArray: the current RHI binds a depth target for a whole texture, with no
	* per array slice view, so an array would need both back ends and the descriptor heaps to grow
	* a slice parameter. An atlas needs only the existing viewport, and it also allows the cascades
	* to be smaller than the first one, which an array cannot express.
	*/
	class RShadowAtlas final
	{
	public:
		// Creates or recreates the depth texture when the edge or the depth bits moved. FALSE when
		// the device refused the resource, in which case the frame draws no shadows.
		BOOL32	EnsureResources(IRRHIDevice* InDevice, const UINT32 InAtlasSize, const UINT32 InDepthBits);
		// Releases the texture through the device, which defers the destroy until the GPU is done.
		void	Release(IRRHIDevice* InDevice);
		// Clears the shelf cursor and every tile. Called once per frame before allocation.
		void	ResetTiles();
		// Places a tile of InSize edge at the shelf cursor. FALSE when it does not fit.
		BOOL32	AllocateTile(const UINT32 InSize, FShadowTile& OutTile);
	public:
		PE_NODISCARD IRRHITexture*	GetTexture()const { return (DepthTexture.IsRenderResourceValid() ? DepthTexture.Texture : nullptr); }
		PE_NODISCARD UINT32			GetAtlasSize()const { return AtlasSize; }
		PE_NODISCARD UINT32			GetDepthBits()const { return DepthBits; }
	private:
		RRHITexture2D	DepthTexture;
		TArray<FShadowTile>	Tiles;
		UINT32	AtlasSize	= 0u;
		UINT32	DepthBits	= 0u;
		// Shelf cursor: where the next tile starts and how tall the current row is.
		UINT32	CursorX		= 0u;
		UINT32	CursorY		= 0u;
		UINT32	RowHeight	= 0u;
	};

	/*
	* Per camera view shadow state: the atlas, the slot table the shading side reads, and the
	* shadow views that feed them.
	*
	* It is keyed by view identity in the renderer, because a cascade belongs to the camera view it
	* was derived from. The slot table holds one element per shadow view plus element zero, the
	* "casts no shadow" sentinel the light lists address with index 0.
	*/
	class RViewShadowState final
	{
	public:
		void	BeginFrame();
		// Finds the view built for a key, or null. The renderer rebuilds a view through this on
		// every frame so its ViewParams block survives across frames.
		PE_NODISCARD RShadowViewProxy*	FindShadowView(const UINT64 InKey)const;
		// Adds a freshly allocated view and returns it. Ownership stays here.
		RShadowViewProxy*				AddShadowView(RShadowViewProxy* InView);
		// Deletes every view that was not marked used this frame.
		void	PruneShadowViews();
		// Builds the slot buffer at the given count (sentinel included) if it is not sized yet,
		// then uploads the staging prefix. The staging array is filled by the renderer.
		BOOL32	EnsureSlotBuffer(IRRHIDevice* InDevice, const UINT32 InTotalSlotNum);
		void	UploadSlots(IRRHIDevice* InDevice);
		void	Release(IRRHIDevice* InDevice);
	public:
		RShadowAtlas				Atlas;
		// One element per slot, index 0 is the sentinel. The shading side addresses it with the
		// shadow map index the light list carries.
		TArray<EShadowSlotGPU>		SlotStaging;
		RStructuredBuffer			SlotBuffer;
		// The shadow views of this frame, one per allocated slot. Only the ones marked used are
		// recorded; PruneShadowViews removes the rest.
		TArray<RShadowViewProxy*>	ShadowViews;
		// Slot number including the sentinel (0 when the view casts no shadows at all).
		UINT32						SlotNum			= 0u;
		// Elements the slot buffer was built for. The buffer never shrinks; a frame with fewer
		// slots uploads a shorter prefix.
		UINT32						SlotCapacity	= 0u;
		// Shadow views the budget or a type cap refused this frame, reported once per frame.
		UINT32						DroppedShadowNum = 0u;
		// TRUE when InitShadowViews ran for this state this frame. A state whose view is no
		// longer a main view keeps its resources but is not rebuilt, and this flag is what
		// tells RenderShadowViews not to record a stale atlas: SlotNum alone cannot, because
		// a view that stopped being main never reaches BeginFrame to reset it.
		BOOL32						bBuiltThisFrame = FALSE;
	};

} // namespace PigeonEngine
