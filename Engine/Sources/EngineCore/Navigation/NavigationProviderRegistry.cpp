#include <Navigation/NavigationProviderRegistry.h>

#include <Navigation/NavigationProviderInterface.h>

namespace PigeonEngine
{

	static TMap<EName, ENavigationProviderRegistry::FCreateFunctionType>& GetProviderFactoryTable()
	{
		// Function local static: the table is constructed on first use, so the order the
		// registering translation units are initialized in does not matter.
		static TMap<EName, ENavigationProviderRegistry::FCreateFunctionType> Table;
		return Table;
	}

	void ENavigationProviderRegistry::Register(const EName& InProviderName, const FCreateFunctionType InCreateFunction)
	{
		if ((!InProviderName.IsValid()) || (InCreateFunction == nullptr))
		{
			PE_FAILED((ENGINE_NAVIGATION_ERROR), ("A navigation provider registration needs a name and a create function."));
			return;
		}
		TMap<EName, FCreateFunctionType>& Table = GetProviderFactoryTable();
		if (Table.FindValueAsPtr(InProviderName) != nullptr)
		{
			PE_FAILED((ENGINE_NAVIGATION_ERROR), ("A navigation provider is already registered under that name."));
			return;
		}
		Table.Add(InProviderName, InCreateFunction);
	}

	ENavigationProviderInterface* ENavigationProviderRegistry::Create(const EName& InProviderName)
	{
		const FCreateFunctionType* CreateFunction = GetProviderFactoryTable().FindValueAsPtr(InProviderName);
		if (CreateFunction == nullptr)
		{
			PE_FAILED((ENGINE_NAVIGATION_ERROR), ("No navigation provider is registered under that name."));
			return nullptr;
		}
		return ((*CreateFunction)());
	}

	BOOL32 ENavigationProviderRegistry::IsRegistered(const EName& InProviderName)
	{
		return (GetProviderFactoryTable().FindValueAsPtr(InProviderName) != nullptr);
	}

	UINT32 ENavigationProviderRegistry::GetRegisteredNum()
	{
		return GetProviderFactoryTable().Num<UINT32>();
	}

}  // namespace PigeonEngine
