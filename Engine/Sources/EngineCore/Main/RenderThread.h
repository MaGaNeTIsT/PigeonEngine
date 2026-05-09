#pragma once

#include <CoreMinimal.h>
#include <MultiThread/Thread.h>
#include <MultiThread/Event.h>
#include <MultiThread/Atomic.h>

namespace PigeonEngine
{

	class IRRHIDevice;
	class RScene;
	class RSceneRenderer;

	/*
	* RenderThread worker for the 1-frame lockstep model.
	*
	* Frame layout (executed each engine update tick):
	*   GameThread (main):                      RenderThread (worker):
	*     World.Tick - emit lambdas to BackSlot
	*     RenderThread.WaitForRenderIdle()  ← waits for RenderDoneEvent
	*     RScene.SwapCommandSlots()
	*     RenderThread.KickRender()          ─→ signals GameDoneEvent
	*                                            SceneRenderer.InitNewFrame
	*                                            SceneRenderer.Render
	*                                            Device.Present
	*                                            ← signals RenderDoneEvent
	*
	* The first call to WaitForRenderIdle returns immediately because
	* RenderDoneEvent is constructed in the signaled state.
	*
	* Threading-wise this model is conservative: D3D11's immediate context
	* is owned exclusively by the worker thread, and the GameThread never
	* touches GPU state. Phase 3 (D3D12 backend) will lift the restriction
	* and allow parallel command-list recording across multiple workers.
	*/
	class RRenderThread final
	{
	public:
		using FAfterRenderCallback = TFunction<void()>;

		RRenderThread(IRRHIDevice* InRenderDevice, RScene* InScene, RSceneRenderer* InSceneRenderer);
		~RRenderThread();

		RRenderThread(const RRenderThread&) = delete;
		RRenderThread(RRenderThread&&) = delete;
		RRenderThread& operator=(const RRenderThread&) = delete;
		RRenderThread& operator=(RRenderThread&&) = delete;

		// Spawns the worker thread. No-op if already started.
		void Start();

		// Signals stop and joins the worker. Safe to call multiple times;
		// also called automatically by the destructor.
		void Stop();

		// GameThread → RenderThread: signal that BackSlot/FrontSlot have
		// been swapped and the worker can render the freshly-published frame.
		void KickRender();

		// GameThread → RenderThread: block until the previous frame's
		// Present has retired. Must be called once per frame before
		// SwapCommandSlots so that the RenderThread is no longer reading
		// the FrontSlot when GameThread reuses it as the next BackSlot.
		void WaitForRenderIdle();

		// Optional callback invoked between SceneRenderer.Render and Present
		// on the RenderThread. Used by editor builds to draw ImGui without
		// touching the immediate context from the GameThread.
		void SetAfterRenderCallback(const FAfterRenderCallback& InCallback);

		BOOL8 IsRunning()const;

	private:
		void WorkerEntry();

	private:
		IRRHIDevice*			RenderDevice;
		RScene*					Scene;
		RSceneRenderer*			SceneRenderer;
		EThread					Worker;
		EEvent					GameDoneEvent;
		EEvent					RenderDoneEvent;
		FAfterRenderCallback	AfterRenderCallback;
		TAtomic<BOOL8>			bStopRequested;
		BOOL8					bStarted;
	};

};
