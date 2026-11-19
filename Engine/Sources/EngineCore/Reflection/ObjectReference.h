#pragma once

#include <CoreMinimal.h>
#include "Archive.h"
#include "Property.h"

namespace PigeonEngine
{
	/*
	* File-format side of an object reference property. The level serializer implements it:
	* saving maps the live handle to a file-local id, loading records a pending reference and
	* applies it after every actor and component exists. Other serializers leave it null and
	* reference properties are skipped.
	*/
	class IObjectReferenceHandler
	{
	public:
		virtual ~IObjectReferenceHandler() = default;
		virtual BOOL32 WriteReference(EArchive& InArchive, const CHAR* InName, const FObjectHandle& InHandle) = 0;
		virtual BOOL32 ReadReference(EArchive& InArchive, const CHAR* InName, const TFunction<void(const FObjectHandle&)>& InApply) = 0;
	};

	// "actorFileId:componentIndex"; componentIndex 0 addresses the actor itself.
	PE_NODISCARD EString MakeObjectReferenceString(const UINT32& InActorFileId, const UINT32& InComponentIndex);
	BOOL32 ParseObjectReferenceString(const EString& InReference, UINT32& OutActorFileId, UINT32& OutComponentIndex);

	/*
	* Registers a property whose value is an object handle. The getter returns the current
	* target (MakeObjectHandle(pointer)); the setter stores the resolved handle. The setter
	* runs only after the whole document exists - never in the middle of loading.
	*/
	template <typename TClass>
	void AddClassObjectReferenceProperty(const size_t& InClassHash, const CHAR* InName,
		const TFunction<FObjectHandle(const TClass*)>& InGetter,
		const TFunction<void(TClass*, const FObjectHandle&)>& InSetter,
		const UINT32& InFlags = static_cast<UINT32>(EPropertyFlagsType::NONE))
	{
		FProperty Property;
		Property.Name = InName;
		Property.Flags = InFlags;
		Property.Serialize = [InName = EString(InName), InGetter, InSetter](EArchive& InArchive, void* InContainer) -> void
		{
			IObjectReferenceHandler* Handler = InArchive.GetObjectReferenceHandler();
			if (!Handler)
			{
				return;
			}
			TClass* Object = static_cast<TClass*>(InContainer);
			if (InArchive.IsSaving())
			{
				Handler->WriteReference(InArchive, *InName, InGetter(Object));
			}
			else
			{
				Handler->ReadReference(InArchive, *InName, [Object, InSetter](const FObjectHandle& InResolved) -> void
				{
					InSetter(Object, InResolved);
				});
			}
		};
		EClassPropertyRegistry::AddProperty(InClassHash, Property);
	}
}
