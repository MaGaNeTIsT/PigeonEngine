#include "ShadowAtlas.h"
#include <RHI/RHITypes.h>

namespace PigeonEngine
{

	// ---------------------------------------------------------------------------
	// RShadowAtlas
	// ---------------------------------------------------------------------------

	BOOL32 RShadowAtlas::EnsureResources(IRRHIDevice* InDevice, const UINT32 InAtlasSize, const UINT32 InDepthBits)
	{
		if (!InDevice)
		{
			return FALSE;
		}
		if ((InAtlasSize == AtlasSize) && (InDepthBits == DepthBits) && (DepthTexture.IsRenderResourceValid() != FALSE))
		{
			return TRUE;
		}
		if (DepthTexture.IsRenderResourceValid() != FALSE)
		{
			// Released through the device, which defers the destroy until the GPU is done with the
			// frames that still reference it.
			DepthTexture.Release(InDevice);
		}
		AtlasSize	= 0u;
		DepthBits	= 0u;
		if (InAtlasSize == 0u)
		{
			return FALSE;
		}

		RTextureDesc AtlasDescription(InAtlasSize, InAtlasSize
			, static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL) | static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)
			, RFormatType::FORMAT_UNKNOWN, nullptr, nullptr, nullptr, InDepthBits);
		const RRHISubresourceData EmptySubresources;
		if (InDevice->CreateTexture2DResource(AtlasDescription, Color4(1.f, 1.f, 1.f, 1.f), EmptySubresources, DepthTexture) != TRUE)
		{
			return FALSE;
		}
		AtlasSize	= InAtlasSize;
		DepthBits	= InDepthBits;
		ResetTiles();
		return TRUE;
	}
	void RShadowAtlas::Release(IRRHIDevice* InDevice)
	{
		if ((InDevice) && (DepthTexture.IsRenderResourceValid() != FALSE))
		{
			DepthTexture.Release(InDevice);
		}
		DepthTexture.ReleaseRenderResource();
		Tiles.Empty();
		AtlasSize	= 0u;
		DepthBits	= 0u;
		CursorX		= 0u;
		CursorY		= 0u;
		RowHeight	= 0u;
	}
	void RShadowAtlas::ResetTiles()
	{
		Tiles.Empty();
		CursorX		= 0u;
		CursorY		= 0u;
		RowHeight	= 0u;
	}
	BOOL32 RShadowAtlas::AllocateTile(const UINT32 InSize, FShadowTile& OutTile)
	{
		if ((InSize == 0u) || (InSize > AtlasSize))
		{
			return FALSE;
		}
		// Shelf: fill the current row, then wrap. The caller feeds sizes in descending order, so a
		// row is as tall as its first tile and the short tiles fill its width.
		//
		// The placement is computed before anything is committed. A refused tile must leave the
		// cursor where it was: the renderer retries a refused size one step smaller, and a cursor
		// that had already wrapped on the attempt would make the retry skip a row that is still
		// empty - or, worse, conclude the atlas is full when it is not.
		UINT32 TileX = CursorX;
		UINT32 TileY = CursorY;
		UINT32 TileRowHeight = RowHeight;
		if ((TileX + InSize) > AtlasSize)
		{
			TileY		+= TileRowHeight;
			TileX		= 0u;
			TileRowHeight = 0u;
		}
		if ((TileY + InSize) > AtlasSize)
		{
			return FALSE;
		}
		OutTile.Rect	= EViewport(static_cast<FLOAT>(TileX), static_cast<FLOAT>(TileY)
			, static_cast<FLOAT>(InSize), static_cast<FLOAT>(InSize)
			, RCommonSettings::RENDER_DEPTH_NEAR, RCommonSettings::RENDER_DEPTH_FAR);
		OutTile.Size	= InSize;
		OutTile.bUsed	= TRUE;
		CursorX			= TileX + InSize;
		CursorY			= TileY;
		RowHeight		= EMath::Max(TileRowHeight, InSize);
		Tiles.Add(OutTile);
		return TRUE;
	}

	// ---------------------------------------------------------------------------
	// RViewShadowState
	// ---------------------------------------------------------------------------

	void RViewShadowState::BeginFrame()
	{
		Atlas.ResetTiles();
		SlotStaging.Empty();
		SlotNum				= 0u;
		DroppedShadowNum	= 0u;
		for (UINT32 ViewIndex = 0u, ViewNum = ShadowViews.Num<UINT32>(); ViewIndex < ViewNum; ViewIndex++)
		{
			ShadowViews[ViewIndex]->ClearUsed();
		}
	}
	RShadowViewProxy* RViewShadowState::FindShadowView(const UINT64 InKey) const
	{
		for (UINT32 ViewIndex = 0u, ViewNum = ShadowViews.Num<UINT32>(); ViewIndex < ViewNum; ViewIndex++)
		{
			if (ShadowViews[ViewIndex]->GetShadowViewKey() == InKey)
			{
				return ShadowViews[ViewIndex];
			}
		}
		return nullptr;
	}
	RShadowViewProxy* RViewShadowState::AddShadowView(RShadowViewProxy* InView)
	{
		if (!InView)
		{
			return nullptr;
		}
		ShadowViews.Add(InView);
		return InView;
	}
	void RViewShadowState::PruneShadowViews()
	{
		// Backwards so an element removal cannot skip the next one.
		for (INT32 ViewIndex = ShadowViews.Num<INT32>() - 1; ViewIndex >= 0; ViewIndex--)
		{
			RShadowViewProxy* View = ShadowViews[ViewIndex];
			if (View && (View->IsUsedThisFrame() != FALSE))
			{
				continue;
			}
			delete View;
			ShadowViews.RemoveAt(ViewIndex);
		}
	}
	BOOL32 RViewShadowState::EnsureSlotBuffer(IRRHIDevice* InDevice, const UINT32 InTotalSlotNum)
	{
		if (!InDevice)
		{
			return FALSE;
		}
		const UINT32 RequiredNum = EMath::Max(InTotalSlotNum, 1u);
		if ((SlotBuffer.IsRenderResourceValid() != FALSE) && (SlotCapacity >= RequiredNum))
		{
			return TRUE;
		}
		SlotBuffer.ReleaseRenderResource();
		const UINT32 ElementSize = static_cast<UINT32>(sizeof(EShadowSlotGPU));
		if (SlotBuffer.InitFromRHI(InDevice
			, RBufferDesc(static_cast<UINT64>(ElementSize) * RequiredNum
				, RBindFlagType::BIND_SHADER_RESOURCE
				, ElementSize
				, RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED)) == FALSE)
		{
			SlotCapacity = 0u;
			return FALSE;
		}
		SlotCapacity = RequiredNum;
		return TRUE;
	}
	void RViewShadowState::UploadSlots(IRRHIDevice* InDevice)
	{
		if ((!InDevice) || (SlotNum == 0u))
		{
			return;
		}
		if (SlotStaging.Num<UINT32>() < SlotNum)
		{
			return;
		}
		SlotBuffer.UploadDataViaRHI(InDevice, SlotStaging.GetData()
			, static_cast<UINT32>(sizeof(EShadowSlotGPU)) * SlotNum, 0u);
	}
	void RViewShadowState::Release(IRRHIDevice* InDevice)
	{
		Atlas.Release(InDevice);
		SlotBuffer.ReleaseRenderResource();
		SlotStaging.Empty();
		SlotNum			= 0u;
		SlotCapacity	= 0u;
		DroppedShadowNum = 0u;
		for (UINT32 ViewIndex = 0u, ViewNum = ShadowViews.Num<UINT32>(); ViewIndex < ViewNum; ViewIndex++)
		{
			delete ShadowViews[ViewIndex];
		}
		ShadowViews.Empty();
	}

} // namespace PigeonEngine
