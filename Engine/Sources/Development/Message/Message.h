#pragma once
#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/Name.h>

namespace PigeonEngine
{
    class EMessageBase
    {
    public:
        explicit EMessageBase(const EName& InMessageName = EName::None());
        virtual ~EMessageBase();

        const EName& GetMessageName()const;
        SIZE_T GetMessageTypeID()const;
        virtual BOOL32 IsObjectMessage()const;
        void SetMessageName(const EName& InMessageName);

    private:
        EName MessageName;
    };

    struct FMessageListenerHandle
    {
    public:
        EName MessageName;
        UINT32 ListenerID = 0u;

        BOOL32 IsValid()const;
        BOOL32 operator==(const FMessageListenerHandle& Other)const;
        BOOL32 operator!=(const FMessageListenerHandle& Other)const;
    };

    class EObjectMessageBase : public EMessageBase
    {
    public:
        EObjectMessageBase(const EName& InMessageName = EName::None(), const EObjectBase* InObject = nullptr);
        virtual ~EObjectMessageBase();

        const EObjectBase* GetObject()const;
        ObjectIdentityType GetObjectUniqueID()const;
        virtual BOOL32 IsObjectMessage()const override;
        void SetObject(const EObjectBase* InObject);

    private:
        const EObjectBase* Object;
    };

    template<typename... TArgs>
    class TMessageArgs final : public EMessageBase
    {
    public:
        using FArgumentTuple = TTuple<TDecayType<TArgs>...>;

    public:
        template<typename... TForwardArgs>
        TMessageArgs(const EName& InMessageName, TForwardArgs&&... InArgs)
            : EMessageBase(InMessageName)
            , Arguments(EMemory::Forward<TForwardArgs>(InArgs)...)
        {
        }

        const FArgumentTuple& GetArguments()const
        {
            return Arguments;
        }

        static SIZE_T GetStaticMessageTypeID()
        {
            return GetClassHashCode<TMessageArgs<TArgs...>>();
        }

    private:
        FArgumentTuple Arguments;
    };

    template<typename... TArgs>
    class TObjectMessageArgs final : public EObjectMessageBase
    {
    public:
        using FArgumentTuple = TTuple<TDecayType<TArgs>...>;

    public:
        template<typename... TForwardArgs>
        TObjectMessageArgs(const EName& InMessageName, const EObjectBase* InObject, TForwardArgs&&... InArgs)
            : EObjectMessageBase(InMessageName, InObject)
            , Arguments(EMemory::Forward<TForwardArgs>(InArgs)...)
        {
        }

        const FArgumentTuple& GetArguments()const
        {
            return Arguments;
        }

        static SIZE_T GetStaticMessageTypeID()
        {
            return GetClassHashCode<TObjectMessageArgs<TArgs...>>();
        }

    private:
        FArgumentTuple Arguments;
    };

    template<typename... TArgs>
    struct TMessageSubscribeArgsHelper
    {
        static constexpr BOOL8 IsSingleMessageType = FALSE;
    };

    template<typename TArg>
    struct TMessageSubscribeArgsHelper<TArg>
    {
        using FCleanArgType = TRemoveCVType<TRemoveRefType<TArg>>;
        static constexpr BOOL8 IsSingleMessageType = TIsBaseOf<EMessageBase, FCleanArgType>::value;
    };

    template<typename... TArgs>
    struct TMessageSendArgsHelper
    {
        static constexpr BOOL8 IsObjectMessageArgs = FALSE;
    };

    template<typename TObjectType, typename... TArgs>
    struct TMessageSendArgsHelper<TObjectType, TArgs...>
    {
        using FCleanObjectPtrType = TDecayType<TObjectType>;
        using FCleanObjectType = TRemoveCVType<TRemovePointerType<FCleanObjectPtrType>>;
        static constexpr BOOL8 IsObjectMessageArgs = TIsPtr<FCleanObjectPtrType>::value && TIsBaseOf<EObjectBase, FCleanObjectType>::value;
    };

    class EMessageManager final : public EManagerBase
    {
    public:
        using FMessageCallback = TFunction<void(const EMessageBase&)>;

    public:
        virtual void Initialize()override;
        virtual void ShutDown()override;

        FMessageListenerHandle Subscribe(const EName& InMessageName, const FMessageCallback& InCallback)
        {
            return SubscribeMessage(InMessageName, InCallback);
        }

        FMessageListenerHandle Subscribe(const EName& InMessageName, const EObjectBase* InObject, const FMessageCallback& InCallback)
        {
            return SubscribeObjectMessage(InMessageName, InObject, InCallback);
        }

        FMessageListenerHandle Subscribe(const EName& InMessageName, const TFunction<void(void)>& InCallback)
        {
            TFunction<void(void)> Callback = InCallback;
            FMessageCallback WrappedCallback;
            WrappedCallback = [Callback](const EMessageBase&) mutable
                {
                    Callback();
                };
            return SubscribeMessage(InMessageName, WrappedCallback);
        }

        template<typename TObjectType>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(void))
        {
            using FCleanObjectType = TRemoveCVType<TRemoveRefType<TObjectType>>;
            Check((TIsBaseOf<EObjectBase, FCleanObjectType>::value), "TObjectType must derive from EObjectBase.");
            Check((InObject != nullptr), "InObject is null.");
            Check((InFunction != nullptr), "InFunction is null.");

            FMessageCallback WrappedCallback;
            WrappedCallback = [InObject, InFunction](const EMessageBase&) mutable
                {
                    (InObject->*InFunction)();
                };

            return SubscribeObjectMessage(InMessageName, static_cast<const EObjectBase*>(InObject), WrappedCallback);
        }

        template<typename TObjectType>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(const EObjectMessageBase&))
        {
            using FCleanObjectType = TRemoveCVType<TRemoveRefType<TObjectType>>;
            Check((TIsBaseOf<EObjectBase, FCleanObjectType>::value), "TObjectType must derive from EObjectBase.");
            Check((InObject != nullptr), "InObject is null.");
            Check((InFunction != nullptr), "InFunction is null.");

            FMessageCallback WrappedCallback;
            WrappedCallback = [InObject, InFunction](const EMessageBase& InMessage) mutable
                {
                    Check((InMessage.IsObjectMessage()), "Message must be an object message.");
                    (InObject->*InFunction)(static_cast<const EObjectMessageBase&>(InMessage));
                };

            return SubscribeObjectMessage(InMessageName, static_cast<const EObjectBase*>(InObject), WrappedCallback);
        }

        template<typename TObjectType, typename TMessageType>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(const TMessageType&))
        {
            using FCleanObjectType = TRemoveCVType<TRemoveRefType<TObjectType>>;
            using FCleanMessageType = TRemoveCVType<TRemoveRefType<TMessageType>>;
            Check((TIsBaseOf<EObjectBase, FCleanObjectType>::value), "TObjectType must derive from EObjectBase.");
            Check((TIsBaseOf<EObjectMessageBase, FCleanMessageType>::value), "TMessageType must derive from EObjectMessageBase.");
            Check((InObject != nullptr), "InObject is null.");
            Check((InFunction != nullptr), "InFunction is null.");

            FMessageCallback WrappedCallback;
            WrappedCallback = [InObject, InFunction](const EMessageBase& InMessage) mutable
                {
                    Check((InMessage.IsObjectMessage()), "Message must be an object message.");
                    (InObject->*InFunction)(static_cast<const FCleanMessageType&>(InMessage));
                };

            return SubscribeObjectMessage(InMessageName, static_cast<const EObjectBase*>(InObject), WrappedCallback);
        }

        template<typename TObjectType, typename... TArgs>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(TArgs...))
        {
            using FCleanObjectType = TRemoveCVType<TRemoveRefType<TObjectType>>;
            using FArgsMessage = TObjectMessageArgs<TDecayType<TArgs>...>;
            Check((TIsBaseOf<EObjectBase, FCleanObjectType>::value), "TObjectType must derive from EObjectBase.");
            Check((InObject != nullptr), "InObject is null.");
            Check((InFunction != nullptr), "InFunction is null.");

            FMessageCallback WrappedCallback;
            WrappedCallback = [InObject, InFunction](const EMessageBase& InMessage) mutable
                {
                    Check((InMessage.IsObjectMessage()), "Message must be an object message.");
                    Check((InMessage.GetMessageTypeID() == FArgsMessage::GetStaticMessageTypeID()), "Object message arguments type mismatch.");
                    const FArgsMessage& ArgsMessage = static_cast<const FArgsMessage&>(InMessage);
                    auto BoundInvoke = [InObject, InFunction](TArgs... InArgs)
                        {
                            (InObject->*InFunction)(InArgs...);
                        };
                    TupleApply(BoundInvoke, ArgsMessage.GetArguments());
                };

            return SubscribeObjectMessage(InMessageName, static_cast<const EObjectBase*>(InObject), WrappedCallback);
        }

        template<typename TObjectType, typename... TArgs>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(const EObjectBase*, TArgs...))
        {
            using FCleanObjectType = TRemoveCVType<TRemoveRefType<TObjectType>>;
            using FArgsMessage = TObjectMessageArgs<TDecayType<TArgs>...>;
            Check((TIsBaseOf<EObjectBase, FCleanObjectType>::value), "TObjectType must derive from EObjectBase.");
            Check((InObject != nullptr), "InObject is null.");
            Check((InFunction != nullptr), "InFunction is null.");

            FMessageCallback WrappedCallback;
            WrappedCallback = [InObject, InFunction](const EMessageBase& InMessage) mutable
                {
                    Check((InMessage.IsObjectMessage()), "Message must be an object message.");
                    Check((InMessage.GetMessageTypeID() == FArgsMessage::GetStaticMessageTypeID()), "Object message arguments type mismatch.");
                    const FArgsMessage& ArgsMessage = static_cast<const FArgsMessage&>(InMessage);
                    auto BoundInvoke = [InObject, InFunction, &ArgsMessage](TArgs... InArgs)
                        {
                            (InObject->*InFunction)(ArgsMessage.GetObject(), InArgs...);
                        };
                    TupleApply(BoundInvoke, ArgsMessage.GetArguments());
                };

            return SubscribeObjectMessage(InMessageName, static_cast<const EObjectBase*>(InObject), WrappedCallback);
        }

        template<typename TCallableType, TEnableIfType<TIsFunctionConstructible<void(void), TCallableType>::value, INT32> = 0>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TCallableType&& InCallback)
        {
            TFunction<void(void)> TypedCallback;
            TypedCallback = EMemory::Forward<TCallableType>(InCallback);
            return (this->*static_cast<FMessageListenerHandle(EMessageManager::*)(const EName&, const TFunction<void(void)>&)>(&EMessageManager::Subscribe))(InMessageName, TypedCallback);
        }

        template<typename TCallableType, TEnableIfType<TIsFunctionConstructible<void(const EMessageBase&), TCallableType>::value, INT32> = 0>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TCallableType&& InCallback)
        {
            FMessageCallback TypedCallback;
            TypedCallback = EMemory::Forward<TCallableType>(InCallback);
            return SubscribeMessage(InMessageName, TypedCallback);
        }

        template<typename TMessageType>
        FMessageListenerHandle Subscribe(const EName& InMessageName, const TFunction<void(const TMessageType&)>& InCallback)
        {
            using FCleanMessageType = TRemoveCVType<TRemoveRefType<TMessageType>>;
            Check((TIsBaseOf<EMessageBase, FCleanMessageType>::value), "TMessageType must derive from EMessageBase.");

            TFunction<void(const FCleanMessageType&)> Callback = InCallback;
            FMessageCallback WrappedCallback;
            WrappedCallback = [Callback](const EMessageBase& InMessage) mutable
                {
                    Callback(static_cast<const FCleanMessageType&>(InMessage));
                };

            return SubscribeMessage(InMessageName, WrappedCallback);
        }

        template<typename... TArgs>
        FMessageListenerHandle Subscribe(const EName& InMessageName, const TFunction<void(TArgs...)>& InCallback)
        {
            using FArgsMessage = TMessageArgs<TDecayType<TArgs>...>;

            TFunction<void(TArgs...)> Callback = InCallback;
            FMessageCallback WrappedCallback;
            WrappedCallback = [Callback](const EMessageBase& InMessage) mutable
                {
                    Check((InMessage.GetMessageTypeID() == FArgsMessage::GetStaticMessageTypeID()), "Message arguments type mismatch.");
                    const FArgsMessage& ArgsMessage = static_cast<const FArgsMessage&>(InMessage);
                    TupleApply(Callback, ArgsMessage.GetArguments());
                };

            return SubscribeMessage(InMessageName, WrappedCallback);
        }

        template<typename TMessageType, typename TCallableType>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TCallableType&& InCallback)
        {
            using FCleanMessageType = TRemoveCVType<TRemoveRefType<TMessageType>>;
            Check((TIsBaseOf<EMessageBase, FCleanMessageType>::value), "TMessageType must derive from EMessageBase.");

            TFunction<void(const FCleanMessageType&)> TypedCallback;
            TypedCallback = EMemory::Forward<TCallableType>(InCallback);
            return (this->*static_cast<FMessageListenerHandle(EMessageManager::*)(const EName&, const TFunction<void(const FCleanMessageType&)>&)>(&EMessageManager::template Subscribe<FCleanMessageType>))(InMessageName, TypedCallback);
        }

        template<typename... TArgs, typename TCallableType, TEnableIfType<((sizeof...(TArgs) > 0) && (!TMessageSubscribeArgsHelper<TArgs...>::IsSingleMessageType)), INT32> = 0>
        FMessageListenerHandle Subscribe(const EName& InMessageName, TCallableType&& InCallback)
        {
            TFunction<void(TArgs...)> TypedCallback;
            TypedCallback = EMemory::Forward<TCallableType>(InCallback);
            return (this->*static_cast<FMessageListenerHandle(EMessageManager::*)(const EName&, const TFunction<void(TArgs...)>&)>(&EMessageManager::template Subscribe<TArgs...>))(InMessageName, TypedCallback);
        }

        BOOL32 Unsubscribe(const FMessageListenerHandle& InHandle)
        {
            if (!InHandle.IsValid())
            {
                return FALSE;
            }

            return Unsubscribe(InHandle.MessageName, InHandle.ListenerID);
        }

        BOOL32 Unsubscribe(const EName& InMessageName, const UINT32 InListenerID);
        BOOL32 Unsubscribe(const EObjectBase* InObject);
        UINT32 Send(const EName& InMessageName);
        UINT32 Send(const EName& InMessageName, const EObjectBase* InObject);
        UINT32 Broadcast(const EMessageBase& InMessage);
        UINT32 Broadcast(const EObjectMessageBase& InMessage);

        template<typename... TArgs, TEnableIfType<!TMessageSendArgsHelper<TArgs...>::IsObjectMessageArgs, INT32> = 0>
        UINT32 Send(const EName& InMessageName, TArgs&&... InArgs)
        {
            using FArgsMessage = TMessageArgs<TDecayType<TArgs>...>;
            return Broadcast(FArgsMessage(InMessageName, EMemory::Forward<TArgs>(InArgs)...));
        }

        template<typename... TArgs>
        UINT32 Send(const EName& InMessageName, const EObjectBase* InObject, TArgs&&... InArgs)
        {
            using FArgsMessage = TObjectMessageArgs<TDecayType<TArgs>...>;
            return Broadcast(FArgsMessage(InMessageName, InObject, EMemory::Forward<TArgs>(InArgs)...));
        }

        template<typename TMessageType>
        UINT32 Broadcast(const TMessageType& InMessage)
        {
            Check((TIsBaseOf<EMessageBase, TMessageType>::value), "TMessageType must derive from EMessageBase.");
            return Broadcast(static_cast<const EMessageBase&>(InMessage));
        }

        BOOL32 HasMessage(const EName& InMessageName)const;
        UINT32 GetListenerNum(const EName& InMessageName)const;
        void ClearMessage(const EName& InMessageName);
        void Clear();

    private:
        FMessageListenerHandle SubscribeMessage(const EName& InMessageName, const FMessageCallback& InCallback);
        FMessageListenerHandle SubscribeObjectMessage(const EName& InMessageName, const EObjectBase* InObject, const FMessageCallback& InCallback);

        struct FMessageListener
        {
        public:
            UINT32 ListenerID = 0u;
            ObjectIdentityType BoundObjectID = 0u;
            FMessageCallback Callback;

            FMessageListener();
            FMessageListener(const UINT32 InListenerID, const FMessageCallback& InCallback);
            FMessageListener(const UINT32 InListenerID, const ObjectIdentityType InBoundObjectID, const FMessageCallback& InCallback);
        };

        TMap<EName, TArray<FMessageListener>> MessageListeners;
        UINT32 NextListenerID = 1u;

        CLASS_MANAGER_SINGLETON_BODY(EMessageManager)
    };
};