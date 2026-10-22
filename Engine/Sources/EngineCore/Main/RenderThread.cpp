#include "RenderThread.h"
#include <RHI/IRRHIDevice.h>
#include <Renderer/RenderScene.h>
#include <Renderer/SceneRenderer.h>
#include <RenderResourceLifetime.h>

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

	void RRenderThread::SetPreSceneRenderCallback(const FAfterRenderCallback& InCallback)
	{
		PreSceneRenderCallback = InCallback;
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

			// Render tier resources are created and destroyed here, before the scene
			// commands run: a proxy added in the same frame then finds the resource its
			// asset asked for already built.
			RRenderResourceManager::GetManagerSingleton()->DoCommands();

			// SceneRenderer drains FrontSlot commands, builds passes,
			// records draws via the IRCommandList, and presents.
			RenderDevice->BeginFrame();
			// Secondary renderers record into their own targets before the main scene does, in
			// the same device frame, so the overlay the main pass submits this frame samples
			// what they wrote this frame rather than the previous one.
			if (PreSceneRenderCallback.IsValid())
			{
				PreSceneRenderCallback();
			}
			SceneRenderer->InitNewFrame();
			SceneRenderer->Render();
			if (AfterRenderCallback.IsValid())
			{
				AfterRenderCallback();
			}
			RenderDevice->Present(0u);
			RenderDevice->EndFrame();

			RenderDoneEvent.Signal();
		}
	}

};
