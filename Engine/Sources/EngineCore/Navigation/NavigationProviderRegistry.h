#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class ENavigationProviderInterface;

	/*
	* Maps a provider name to the function that creates it.
	*
	* This is the same shape as RPrimitiveSceneProxyFactory: a backend registers itself from
	* the static registration hook of its own translation unit, a component asks for it by
	* name, and nothing at the call site knows the concrete type. Replacing or adding a
	* navigation backend is therefore a registration, not a switch statement.
	*/
	class ENavigationProviderRegistry
	{
	public:
		typedef ENavigationProviderInterface* (*FCreateFunctionType)();
	public:
		// Refuses a duplicate name: two providers under one name would make which one a
		// component gets depend on registration order.
		static void Register(const EName& InProviderName, const FCreateFunctionType InCreateFunction);
		// Returns a new provider, or nullptr when nothing is registered under that name.
		PE_NODISCARD static ENavigationProviderInterface* Create(const EName& InProviderName);
		PE_NODISCARD static BOOL32 IsRegistered(const EName& InProviderName);
		PE_NODISCARD static UINT32 GetRegisteredNum();
	};

}  // namespace PigeonEngine
