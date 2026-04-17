#include "Message.h"

namespace PigeonEngine
{
    EMessageBase::EMessageBase(const EName& InMessageName)
        : MessageName(InMessageName)
    {
    }

    EMessageBase::~EMessageBase() = default;

    const EName& EMessageBase::GetMessageName()const
    {
        return MessageName;
    }

    SIZE_T EMessageBase::GetMessageTypeID()const
    {
        return GetClassHashCode(this);
    }

    BOOL32 EMessageBase::IsObjectMessage()const
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

    const EObjectBase* EObjectMessageBase::GetObject()const
    {
        return Object;
    }

    ObjectIdentityType EObjectMessageBase::GetObjectUniqueID()const
    {
        if (Object)
        {
            return Object->GetUniqueID();
        }
        return 0u;
    }

    BOOL32 EObjectMessageBase::IsObjectMessage()const
    {
        return TRUE;
    }

    void EObjectMessageBase::SetObject(const EObjectBase* InObject)
    {
        Object = InObject;
    }

    BOOL32 FMessageListenerHandle::IsValid()const
    {
        return (MessageName.IsValid() && (ListenerID > 0u));
    }

    BOOL32 FMessageListenerHandle::operator==(const FMessageListenerHandle& Other)const
    {
        return ((MessageName == Other.MessageName) && (ListenerID == Other.ListenerID));
    }

    BOOL32 FMessageListenerHandle::operator!=(const FMessageListenerHandle& Other)const
    {
        return (!((*this) == Other));
    }

    void EMessageManager::Initialize()
    {
        Clear();
        NextListenerID = 1u;
    }

    void EMessageManager::ShutDown()
    {
        Clear();
    }

    FMessageListenerHandle EMessageManager::SubscribeMessage(const EName& InMessageName, const FMessageCallback& InCallback)
    {
        FMessageListenerHandle Result;
        if (!InMessageName.IsValid())
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
        if (!InMessageName.IsValid() || (InObject == nullptr))
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
        if (!InObject)
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

    BOOL32 EMessageManager::HasMessage(const EName& InMessageName)const
    {
        const TArray<FMessageListener>* ListenerList = MessageListeners.FindValueAsPtr(InMessageName);
        return ((ListenerList != nullptr) && (!ListenerList->IsEmpty()));
    }

    UINT32 EMessageManager::GetListenerNum(const EName& InMessageName)const
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