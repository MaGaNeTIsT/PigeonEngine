#include "RenderThread.h"
#include <RHI/IRRHIDevice.h>
#include <Renderer/RenderScene.h>
#include <Renderer/SceneRenderer.h>

namespace PigeonEngine
{

	RRenderThread::RRenderThread(IRRHIDevice* InRenderDevice, RScene* InScene, RSceneRenderer* InSceneRenderer)
		: RenderDevice(InRenderDevice)
		, Scene(InScene)
		, SceneRenderer(InSceneRenderer)
		, Worker(EString("PigeonRenderThread"))
		, GameDoneEvent(FALSE)
		, RenderDoneEvent(TRUE)		// initially signaled so the first WaitForRenderIdle returns immediately
		, bStopRequested(FALSE)
		, bStarted(FALSE)
	{
	}

	RRenderThread::~RRenderThread()
	{
		Stop();
	}

	void RRenderThread::Start()
	{
		if (bStarted)
		{
			return;
		}
		bStopRequested.Store(FALSE);
		bStarted = TRUE;
		Worker.Start([this]() -> void
		{
			this->WorkerEntry();
		});
	}

	void RRenderThread::Stop()
	{
		if (!bStarted)
		{
			return;
		}
		bStopRequested.Store(TRUE);
		// Wake the worker so it can observe the stop flag even if no game
		// frame is currently in flight.
		GameDoneEvent.Signal();
		Worker.Join();
		bStarted = FALSE;
	}

	void RRenderThread::KickRender()
	{
		GameDoneEvent.Signal();
	}

	void RRenderThread::WaitForRenderIdle()
	{
		RenderDoneEvent.Wait();
		RenderDoneEvent.Reset();
	}

	void RRenderThread::SetAfterRenderCallback(const FAfterRenderCallback& InCallback)
	{
		AfterRenderCallback = InCallback;
	}

	BOOL8 RRenderThread::IsRunning()const
	{
		return bStarted;
	}

	void RRenderThread::WorkerEntry()
	{
		while (!bStopRequested.Load())
		{
			GameDoneEvent.Wait();
			GameDoneEvent.Reset();

			if (bStopRequested.Load())
			{
				break;
			}

			// SceneRenderer drains FrontSlot commands, builds passes,
			// records draws via the IRCommandList, and presents.
			SceneRenderer->InitNewFrame();
			SceneRenderer->Render();
			if (AfterRenderCallback.IsValid())
			{
				AfterRenderCallback();
			}
			RenderDevice->Present(0u);

			RenderDoneEvent.Signal();
		}
	}

};
