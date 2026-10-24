#include "AnimationNotify.h"

#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	namespace
	{
		TMap<EString, EAnimNotifyRegistry::EAnimNotifyFactory>& GetNotifyRegistry()
		{
			// Function-local so registration order between translation units is not a problem.
			static TMap<EString, EAnimNotifyRegistry::EAnimNotifyFactory> Registry;
			return Registry;
		}

		/*
		* The default notify is a log line. It exists so the editor has something selectable
		* and the runtime path can be exercised before game-specific notify classes are added.
		*/
		class EAnimNotify_Log final : public EAnimNotify
		{
		public:
			virtual void Notify(EAnimInstance& InAnimInstance, const FAnimNotifyEvent& InEvent) override
			{
				(void)InAnimInstance;
				PE_LOG_LOG(EString("AnimNotify '") + InEvent.NotifyName
					+ EString("' at ") + ToString(InEvent.Time) + EString("s."));
			}
		};

		/*
		* Default state notify used for authoring and for exercising Begin/End before
		* game-specific windows exist. Tick is intentionally silent: logging every frame
		* would drown the log, and Begin/End already prove the active range.
		*/
		class EAnimNotifyState_Log final : public EAnimNotifyState
		{
		public:
			virtual void NotifyBegin(EAnimInstance& InAnimInstance, const FAnimNotifyEvent& InEvent) override
			{
				(void)InAnimInstance;
				PE_LOG_LOG(EString("AnimNotifyState '") + InEvent.NotifyName
					+ EString("' begin at ") + ToString(InEvent.Time) + EString("s."));
			}
			virtual void NotifyEnd(EAnimInstance& InAnimInstance, const FAnimNotifyEvent& InEvent) override
			{
				(void)InAnimInstance;
				PE_LOG_LOG(EString("AnimNotifyState '") + InEvent.NotifyName + EString("' end."));
			}
		};

		BOOL32 RegisterDefaultNotifies()
		{
			EAnimNotifyRegistry::Register(EString("Log"), []() -> EAnimNotify*
				{
					return new EAnimNotify_Log();
				});
			EAnimNotifyRegistry::Register(EString("LogState"), []() -> EAnimNotify*
				{
					return new EAnimNotifyState_Log();
				});
			return TRUE;
		}

		[[maybe_unused]] const BOOL32 GDefaultNotifiesRegistered = RegisterDefaultNotifies();
	}

	void FAnimNotifyEvent::Compile()
	{
		if (CachedNotify)
		{
			return;
		}
		if (NotifyClassName.Length() <= 0u)
		{
			return;
		}
		CachedNotify = EAnimNotifyRegistry::Create(NotifyClassName);
		if (!CachedNotify)
		{
			PE_LOG_WARN(EString("AnimNotify '") + NotifyName + EString("' uses unregistered class '")
				+ NotifyClassName + EString("'."));
			return;
		}
		if (bIsState != CachedNotify->IsStateNotify())
		{
			// Mixing the two kinds would make the event silently do nothing (a state event
			// with an instant class never gets Begin, or an instant event with a state class
			// has no Notify implementation). Reject it at compile time instead.
			PE_LOG_WARN(EString("AnimNotify '") + NotifyName + EString("' class '")
				+ NotifyClassName + EString("' is ") + (bIsState ? EString("not a state notify.") : EString("a state notify but the event is instant.")));
			CachedNotify.reset();
		}
	}

	void FAnimNotifyEvent::ResetCompiledNotify()
	{
		CachedNotify.reset();
	}

	BOOL32 EAnimNotifyRegistry::Register(const EString& InClassName, EAnimNotifyFactory InFactory)
	{
		if ((InClassName.Length() <= 0u) || (!InFactory))
		{
			return FALSE;
		}
		TMap<EString, EAnimNotifyFactory>& Registry = GetNotifyRegistry();
		if (Registry.ContainsKey(InClassName))
		{
			return FALSE;
		}
		Registry.Add(InClassName, InFactory);
		return TRUE;
	}

	TSharedPtr<EAnimNotify> EAnimNotifyRegistry::Create(const EString& InClassName)
	{
		TMap<EString, EAnimNotifyFactory>& Registry = GetNotifyRegistry();
		EAnimNotifyFactory* Factory = Registry.FindValueAsPtr(InClassName);
		if ((!Factory) || (!(*Factory)))
		{
			return nullptr;
		}
		return TSharedPtr<EAnimNotify>((*Factory)());
	}

	TArray<EString> EAnimNotifyRegistry::GetRegisteredClassNames()
	{
		TArray<EString> Result;
		TMap<EString, EAnimNotifyFactory>& Registry = GetNotifyRegistry();
		for (auto It = Registry.Begin(); It != Registry.End(); It++)
		{
			Result.Add(It->first);
		}
		return Result;
	}

}
