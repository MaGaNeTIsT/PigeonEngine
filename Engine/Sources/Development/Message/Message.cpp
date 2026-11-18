#include "Message.h"
#include <PigeonBase/Object/Object.h>

/*
* Message bus contract:
*   - Game thread only. Subscribe/Unsubscribe/Send/Broadcast are not synchronized and
*     are expected to be called from the game thread, the owner of every PObject.
*   - Object-bound subscriptions die with their owner. When a PObject is destroyed,
*     PObject::OnObjectPreDelete fires and the bus unsubscribes that object, so a
*     component only has to keep the returned FMessageListenerHandle - no manual
*     unsubscription is required for safety. Unsubscribe(handle) remains available for
*     "stop receiving earlier" in Uninit.
*   - A plain lambda subscription without an owner is NOT covered by that cleanup.
*     Long-lived component code must subscribe through the object-bound overloads
*     (passing `this`), or unsubscribe itself.
*   - Dispatch is synchronous and walks a copy of the listener list, so listeners may
*     subscribe/unsubscribe from inside a callback. The copy is also why messages are
*     meant for occasional broadcasts, not per-frame hot paths.
*/
namespace PigeonEngine
{
	EMessageBase::EMessageBase(const EName& InMessageName)
		: MessageName(InMessageName)
	{
	}

	EMessageBase::~EMessageBase() = default;

	const EName& EMessageBase::GetMessageName() const
	{
		return MessageName;
	}

	SIZE_T EMessageBase::GetMessageTypeID() const
	{
		return GetClassHashCode(this);
	}

	BOOL32 EMessageBase::IsObjectMessage() const
	{
		return FALSE;
	}

	void EMessageBase::SetMessageName(const EName& InMessageName)
	{
		MessageName = InMessageName;
	}

	EObjectMessageBase::EObjectMessageBase(const EName& InMessageName, const EObjectBase* InObject)
		: EMessageBase(InMessageName)
		, Object(InObject)
	{
	}

	EObjectMessageBase::~EObjectMessageBase() = default;

	const EObjectBase* EObjectMessageBase::GetObject() const
	{
		return Object;
	}

	ObjectIdentityType EObjectMessageBase::GetObjectUniqueID() const
	{
		if (Object)
		{
			return Object->GetUniqueID();
		}
		return 0u;
	}

	BOOL32 EObjectMessageBase::IsObjectMessage() const
	{
		return TRUE;
	}

	void EObjectMessageBase::SetObject(const EObjectBase* InObject)
	{
		Object = InObject;
	}

	BOOL32 FMessageListenerHandle::IsValid() const
	{
		return (MessageName.IsValid() && (ListenerID > 0u));
	}

	BOOL32 FMessageListenerHandle::operator==(const FMessageListenerHandle& Other) const
	{
		return ((MessageName == Other.MessageName) && (ListenerID == Other.ListenerID));
	}

	BOOL32 FMessageListenerHandle::operator!=(const FMessageListenerHandle& Other) const
	{
		return (!((*this) == Other));
	}

	void EMessageManager::Initialize()
	{
		Clear();
		NextListenerID = 1u;
		bIsShutDown.Store(FALSE);
		// The bus is the first global observer of the object lifecycle: a destroyed
		// object's subscriptions must go with it, and OnObjectPreDelete is the one point
		// every PObject destroy path passes while the object is still valid.
		ObjectPreDeleteHandle = PObject::OnObjectPreDelete.Add(
			TFunction<void(PObject*)>([this](PObject* Object) -> void
			{
				this->Unsubscribe(Object);
			}));
	}

	void EMessageManager::ShutDown()
	{
		// Off the lifecycle hook first: an object destroyed after this point must not
		// call back into a bus that is closing.
		if (ObjectPreDeleteHandle != 0u)
		{
			PObject::OnObjectPreDelete.Remove(ObjectPreDeleteHandle);
			ObjectPreDeleteHandle = 0u;
		}
		bIsShutDown.Store(TRUE);
		Clear();
		NextListenerID = 1u;
	}

	FMessageListenerHandle EMessageManager::SubscribeMessage(const EName& InMessageName, const FMessageCallback& InCallback)
	{
		FMessageListenerHandle Result;
		if ((bIsShutDown.Load() != FALSE) || (!InMessageName.IsValid()))
		{
			return Result;
		}

		TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
		if (!ListenerList)
		{
			MessageListeners.Add(InMessageName, TArray<FMessageListener>());
			ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
		}

		if (!ListenerList)
		{
			return Result;
		}

		const UINT32 NewListenerID = NextListenerID++;
		ListenerList->Add(FMessageListener(NewListenerID, InCallback));

		Result.MessageName = InMessageName;
		Result.ListenerID = NewListenerID;
		return Result;
	}

	FMessageListenerHandle EMessageManager::SubscribeObjectMessage(const EName& InMessageName, const EObjectBase* InObject, const FMessageCallback& InCallback)
	{
		FMessageListenerHandle Result;
		if ((bIsShutDown.Load() != FALSE) || (!InMessageName.IsValid()) || (InObject == nullptr))
		{
			return Result;
		}

		TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
		if (!ListenerList)
		{
			MessageListeners.Add(InMessageName, TArray<FMessageListener>());
			ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
		}

		if (!ListenerList)
		{
			return Result;
		}

		const UINT32 NewListenerID = NextListenerID++;
		ListenerList->Add(FMessageListener(NewListenerID, InObject->GetUniqueID(), InCallback));

		Result.MessageName = InMessageName;
		Result.ListenerID = NewListenerID;
		return Result;
	}

	BOOL32 EMessageManager::Unsubscribe(const EName& InMessageName, const UINT32 InListenerID)
	{
		TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
		if (!ListenerList)
		{
			return FALSE;
		}

		for (INT32 i = ListenerList->Num<INT32>() - 1; i >= 0; i--)
		{
			if ((*ListenerList)[i].ListenerID == InListenerID)
			{
				ListenerList->RemoveAt(i);
				if (ListenerList->IsEmpty())
				{
					MessageListeners.Remove(InMessageName);
				}
				return TRUE;
			}
		}

		return FALSE;
	}

	BOOL32 EMessageManager::Unsubscribe(const EObjectBase* InObject)
	{
		// Called from PObject::OnObjectPreDelete for every destroyed object, so the empty
		// case is a hot path and must cost one branch.
		if ((!InObject) || (MessageListeners.Num<UINT32>() == 0u))
		{
			return FALSE;
		}

		const ObjectIdentityType BoundObjectID = InObject->GetUniqueID();
		BOOL32 Result = FALSE;
		TArray<EName> MessageNames;
		MessageListeners.GenerateKeyArray(MessageNames);
		for (UINT32 MessageIndex = 0u; MessageIndex < MessageNames.Num<UINT32>(); MessageIndex++)
		{
			TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(MessageNames[MessageIndex]);
			if (!ListenerList)
			{
				continue;
			}

			for (INT32 i = ListenerList->Num<INT32>() - 1; i >= 0; i--)
			{
				if ((*ListenerList)[i].BoundObjectID == BoundObjectID)
				{
					ListenerList->RemoveAt(i);
					Result = TRUE;
				}
			}

			if (ListenerList->IsEmpty())
			{
				MessageListeners.Remove(MessageNames[MessageIndex]);
			}
		}

		return Result;
	}

	UINT32 EMessageManager::Send(const EName& InMessageName)
	{
		return Broadcast(EMessageBase(InMessageName));
	}

	UINT32 EMessageManager::Broadcast(const EMessageBase& InMessage)
	{
		if (bIsShutDown.Load() != FALSE)
		{
			return 0u;
		}

		const TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(InMessage.GetMessageName());
		if (!ListenerList || ListenerList->IsEmpty())
		{
			return 0u;
		}

		TArray<FMessageListener> CallingListeners(*ListenerList);
		const UINT32 ListenerNum = CallingListeners.Num<UINT32>();
		for (UINT32 i = 0u; i < ListenerNum; i++)
		{
			CallingListeners[i].Callback(InMessage);
		}

		return ListenerNum;
	}

	UINT32 EMessageManager::Send(const EName& InMessageName, const EObjectBase* InObject)
	{
		return Broadcast(EObjectMessageBase(InMessageName, InObject));
	}

	UINT32 EMessageManager::Broadcast(const EObjectMessageBase& InMessage)
	{
		return Broadcast(static_cast<const EMessageBase&>(InMessage));
	}

	BOOL32 EMessageManager::HasMessage(const EName& InMessageName) const
	{
		const TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
		return ((ListenerList != nullptr) && (!ListenerList->IsEmpty()));
	}

	UINT32 EMessageManager::GetListenerNum(const EName& InMessageName) const
	{
		const TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
		if (!ListenerList)
		{
			return 0u;
		}

		return ListenerList->Num<UINT32>();
	}

	void EMessageManager::ClearMessage(const EName& InMessageName)
	{
		MessageListeners.Remove(InMessageName);
	}

	void EMessageManager::Clear()
	{
		MessageListeners.Empty();
	}

	EMessageManager::FMessageListener::FMessageListener() = default;

	EMessageManager::FMessageListener::FMessageListener(const UINT32 InListenerID, const FMessageCallback& InCallback)
		: ListenerID(InListenerID)
		, Callback(InCallback)
	{
	}

	EMessageManager::FMessageListener::FMessageListener(const UINT32 InListenerID, const ObjectIdentityType InBoundObjectID, const FMessageCallback& InCallback)
		: ListenerID(InListenerID)
		, BoundObjectID(InBoundObjectID)
		, Callback(InCallback)
	{
	}

	EMessageManager::EMessageManager() = default;
	EMessageManager::~EMessageManager() = default;
};
