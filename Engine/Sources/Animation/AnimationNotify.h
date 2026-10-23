#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class EAnimInstance;
	class EAnimNotify;
	struct FAnimNotifyEvent;

	/*
	* Base class for an animation event. A notify is a small, stateless (or caller-owned)
	* object that runs gameplay code when a clip reaches its time: play a sound, spawn an
	* effect, open a combat window, and so on.
	*
	* The event data lives in the animation asset; the notify object is created from the
	* registered class when the asset is loaded and cached in the event, so playback never
	* parses a class name or allocates a notify.
	*
	* The cached object is shared by every character playing the clip. Keep notify classes
	* stateless; per-play state belongs in the anim instance or the gameplay object.
	*/
	class EAnimNotify
	{
	public:
		virtual ~EAnimNotify() = default;
		virtual void Notify(EAnimInstance& InAnimInstance, const FAnimNotifyEvent& InEvent) {}
	};

	/*
	* One authored event inside a clip. Time is in seconds from the start of the clip.
	* PayloadJson is free-form data for the notify class (sound name, socket, effect name,
	* gameplay tag, ...). It is stored as text so the editor can edit it without the asset
	* format having to know every notify type.
	*
	* CachedNotify is runtime-only: the asset loader compiles it from NotifyClassName.
	*/
	struct FAnimNotifyEvent
	{
	public:
		EString					NotifyName;
		EString					NotifyClassName;
		FLOAT					Time = 0.0f;
		EString					PayloadJson;
		TSharedPtr<EAnimNotify>	CachedNotify;

	public:
		// Creates CachedNotify when it is empty and a class name is present. Safe to call
		// more than once; a missing class only leaves the cache empty and logs a warning.
		void Compile();
		// Drops the cached object so the next Compile() recreates it after an edit.
		void ResetCompiledNotify();
	};

	/*
	* Class-name to factory table for notify types. Registration is explicit so an editor
	* build can list the available classes and an asset only has to persist the class name.
	*/
	class EAnimNotifyRegistry
	{
	public:
		typedef TFunction<EAnimNotify*()> EAnimNotifyFactory;
	public:
		static BOOL32 Register(const EString& InClassName, EAnimNotifyFactory InFactory);
		static TSharedPtr<EAnimNotify> Create(const EString& InClassName);
		PE_NODISCARD static TArray<EString> GetRegisteredClassNames();
	};

}
