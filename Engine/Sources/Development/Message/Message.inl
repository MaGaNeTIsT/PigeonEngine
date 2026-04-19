template<typename... TArgs>
template<typename... TForwardArgs>
TMessageArgs<TArgs...>::TMessageArgs(const EName& InMessageName, TForwardArgs&&... InArgs)
    : EMessageBase(InMessageName)
    , Arguments(EMemory::Forward<TForwardArgs>(InArgs)...)
{
}

template<typename... TArgs>
const typename TMessageArgs<TArgs...>::FArgumentTuple& TMessageArgs<TArgs...>::GetArguments()const
{
    return Arguments;
}

template<typename... TArgs>
SIZE_T TMessageArgs<TArgs...>::GetStaticMessageTypeID()
{
    return GetClassHashCode<TMessageArgs<TArgs...>>();
}

template<typename... TArgs>
template<typename... TForwardArgs>
TObjectMessageArgs<TArgs...>::TObjectMessageArgs(const EName& InMessageName, const EObjectBase* InObject, TForwardArgs&&... InArgs)
    : EObjectMessageBase(InMessageName, InObject)
    , Arguments(EMemory::Forward<TForwardArgs>(InArgs)...)
{
}

template<typename... TArgs>
const typename TObjectMessageArgs<TArgs...>::FArgumentTuple& TObjectMessageArgs<TArgs...>::GetArguments()const
{
    return Arguments;
}

template<typename... TArgs>
SIZE_T TObjectMessageArgs<TArgs...>::GetStaticMessageTypeID()
{
    return GetClassHashCode<TObjectMessageArgs<TArgs...>>();
}

template<typename TObjectType>
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(void))
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
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(const EObjectMessageBase&))
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
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(const TMessageType&))
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
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(TArgs...))
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
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TObjectType* InObject, void(TObjectType::*InFunction)(const EObjectBase*, TArgs...))
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

template<typename TCallableType, TEnableIfType<TIsFunctionConstructible<void(void), TCallableType>::value, INT32>>
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TCallableType&& InCallback)
{
    TFunction<void(void)> TypedCallback;
    TypedCallback = EMemory::Forward<TCallableType>(InCallback);
    return (this->*static_cast<FMessageListenerHandle(EMessageManager::*)(const EName&, const TFunction<void(void)>&)>(&EMessageManager::Subscribe))(InMessageName, TypedCallback);
}

template<typename TCallableType, TEnableIfType<TIsFunctionConstructible<void(const EMessageBase&), TCallableType>::value, INT32>>
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TCallableType&& InCallback)
{
    FMessageCallback TypedCallback;
    TypedCallback = EMemory::Forward<TCallableType>(InCallback);
    return SubscribeMessage(InMessageName, TypedCallback);
}

template<typename TMessageType>
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, const TFunction<void(const TMessageType&)>& InCallback)
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
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, const TFunction<void(TArgs...)>& InCallback)
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
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TCallableType&& InCallback)
{
    using FCleanMessageType = TRemoveCVType<TRemoveRefType<TMessageType>>;
    Check((TIsBaseOf<EMessageBase, FCleanMessageType>::value), "TMessageType must derive from EMessageBase.");

    TFunction<void(const FCleanMessageType&)> TypedCallback;
    TypedCallback = EMemory::Forward<TCallableType>(InCallback);
    return (this->*static_cast<FMessageListenerHandle(EMessageManager::*)(const EName&, const TFunction<void(const FCleanMessageType&)>&)>(&EMessageManager::template Subscribe<FCleanMessageType>))(InMessageName, TypedCallback);
}

template<typename... TArgs, typename TCallableType, TEnableIfType<((sizeof...(TArgs) > 0) && (!TMessageSubscribeArgsHelper<TArgs...>::IsSingleMessageType)), INT32>>
FMessageListenerHandle EMessageManager::Subscribe(const EName& InMessageName, TCallableType&& InCallback)
{
    TFunction<void(TArgs...)> TypedCallback;
    TypedCallback = EMemory::Forward<TCallableType>(InCallback);
    return (this->*static_cast<FMessageListenerHandle(EMessageManager::*)(const EName&, const TFunction<void(TArgs...)>&)>(&EMessageManager::template Subscribe<TArgs...>))(InMessageName, TypedCallback);
}

template<typename... TArgs, TEnableIfType<!TMessageSendArgsHelper<TArgs...>::IsObjectMessageArgs, INT32>>
UINT32 EMessageManager::Send(const EName& InMessageName, TArgs&&... InArgs)
{
    using FArgsMessage = TMessageArgs<TDecayType<TArgs>...>;
    return Broadcast(FArgsMessage(InMessageName, EMemory::Forward<TArgs>(InArgs)...));
}

template<typename... TArgs>
UINT32 EMessageManager::Send(const EName& InMessageName, const EObjectBase* InObject, TArgs&&... InArgs)
{
    using FArgsMessage = TObjectMessageArgs<TDecayType<TArgs>...>;
    return Broadcast(FArgsMessage(InMessageName, InObject, EMemory::Forward<TArgs>(InArgs)...));
}

template<typename TMessageType>
UINT32 EMessageManager::Broadcast(const TMessageType& InMessage)
{
    Check((TIsBaseOf<EMessageBase, TMessageType>::value), "TMessageType must derive from EMessageBase.");
    return Broadcast(static_cast<const EMessageBase&>(InMessage));
}
