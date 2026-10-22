#pragma once

#include <CoreMinimal.h>
#include <RHI/RHITypes.h>

namespace PigeonEngine
{
	class IRRHITexture;

	/*
	* A swap chain that presents into a window other than the engine's main one.
	*
	* ImGui's multi-viewport support is the reason it exists: a panel dragged out of the main
	* window becomes an OS window, and that window needs a back buffer and a Present of its own.
	*
	* The main window's swap chain is not one of these. It is created by the device during
	* Initialize and reached through IRRHIDevice::GetCurrentBackBuffer / Present; this interface
	* covers the extra windows only, so the main path is untouched.
	*
	* Ownership: IRRHIDevice::CreateSwapChainForWindow returns a chain the caller deletes. A
	* chain must be released before the device it came from, and while no submitted command
	* list can still be presenting it.
	*/
	class IRRHISwapChain
	{
	public:
		virtual ~IRRHISwapChain() = default;

		// The back buffer to record the window's frame into. On a chain with more than one
		// buffer this is the one the platform says is current.
		virtual IRRHITexture*	GetBackBuffer() = 0;

		// Recreates the back buffers at the new size. The OS window is resized by the caller;
		// this only follows it.
		virtual BOOL8			Resize(UINT32 InWidth, UINT32 InHeight) = 0;

		virtual void			Present(UINT32 InSyncInterval) = 0;
	};
};
