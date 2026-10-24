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

		/*
		* State notifies are active for a time range instead of one frame. A state class
		* overrides IsStateNotify() and the Begin/Tick/End hooks; an instant class leaves
		* them at these defaults. The asset stores which kind the event is, and Compile()
		* rejects a class whose kind does not match the event.
		*/
		virtual BOOL32 IsStateNotify()const { return FALSE; }
		virtual void NotifyBegin(EAnimInstance& InAnimInstance, const FAnimNotifyEvent& InEvent) {}
		virtual void NotifyTick(EAnimInstance& InAnimInstance, const FAnimNotifyEvent& InEvent, FLOAT InDeltaSeconds) {}
		virtual void NotifyEnd(EAnimInstance& InAnimInstance, const FAnimNotifyEvent& InEvent) {}
	};

	/*
	* Base class for a time-range notify: a hit window, invulnerability, a weapon trail,
	* a looping sound, and so on. The registry factory returns the concrete subclass.
	*/
	class EAnimNotifyState : public EAnimNotify
	{
	public:
		virtual BOOL32 IsStateNotify()const override { return TRUE; }
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
		// Optional reference to a standalone notify asset. When set, the asset supplies
		// class, state kind, default payload and duration; per-event duration/payload can
		// still override them.
		EString					NotifyAssetName;
		FLOAT					Time = 0.0f;
		EString					PayloadJson;
		// State events are active over [Time, Time + StateDuration). Instant events leave
		// bIsState false and only use Time.
		BOOL32					bIsState = FALSE;
		FLOAT					StateDuration = 0.0f;
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
