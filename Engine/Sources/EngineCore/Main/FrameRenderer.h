#pragma once

namespace PigeonEngine
{
	class EName;

	/*
	* A renderer that records a frame of its own beside the main scene renderer.
	*
	* The editor asset previews are the users: each one owns a world, a scene renderer and an
	* offscreen target, and none of that may be recorded on the GameThread - the device belongs
	* to the render thread. Registering with EMainManager is what puts such a renderer in the
	* frame: PublishFrameCommands runs at the frame boundary, where the main scene's command
	* slots are swapped, and RenderFrame runs on the render thread inside the same device frame
	* but before the main scene records, so a texture a preview writes is one the frame's ImGui
	* overlay can sample.
	*
	* These calls run with no other thread inside the device, which is why registration and
	* unregistration from the GameThread need no lock.
	*/
	class IEngineFrameRenderer
	{
	public:
		virtual ~IEngineFrameRenderer() = default;

		// GameThread: once per frame, at the main scene's command-slot swap. The frame's game
		// side has already published its commands into the back slot when this runs.
		virtual void	PublishFrameCommands() = 0;

		// RenderThread: once per frame, after the device frame opened and before the main
		// scene records its own passes.
		virtual void	RenderFrame() = 0;

		// RenderThread: a shader asset was replaced, so everything derived from the previous
		// object is stale. The main scene renderer is told directly; a secondary renderer is
		// told through here. The default drops nothing, which is what a renderer with no
		// shader derived state needs.
		virtual void	OnShaderAssetReplaced(const EName& InShaderName)
		{
			(void)(InShaderName);
		}
	};
};
