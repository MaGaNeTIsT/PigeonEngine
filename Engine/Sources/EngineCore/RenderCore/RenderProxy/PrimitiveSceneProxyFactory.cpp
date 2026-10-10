#include "PrimitiveSceneProxyFactory.h"

#include <RenderProxy/PrimitiveSceneProxy.h>

namespace PigeonEngine
{

	static TMap<EName, RPrimitiveSceneProxyFactory::FCreateFunctionType>& GetProxyFactoryTable()
	{
		// A function local static: the table is built on first use, whichever order the
		// translation units that register into it happen to be initialized in.
		static TMap<EName, RPrimitiveSceneProxyFactory::FCreateFunctionType> Table;
		return Table;
	}

	void RPrimitiveSceneProxyFactory::Register(const EName& InProxyTypeName, const FCreateFunctionType InCreateFunction)
	{
		if ((!InProxyTypeName.IsValid()) || (InCreateFunction == nullptr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("A primitive scene proxy registration needs a name and a create function."));
			return;
		}
		TMap<EName, FCreateFunctionType>& Table = GetProxyFactoryTable();
		if (Table.FindValueAsPtr(InProxyTypeName) != nullptr)
		{
			// Two proxies under one name would make which one a primitive gets depend on
			// registration order, so it is refused rather than silently overwritten.
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("A primitive scene proxy is already registered under that name."));
			return;
		}
		Table.Add(InProxyTypeName, InCreateFunction);
	}

	RPrimitiveSceneProxy* RPrimitiveSceneProxyFactory::Create(const EName& InProxyTypeName)
	{
		const FCreateFunctionType* CreateFunction = GetProxyFactoryTable().FindValueAsPtr(InProxyTypeName);
		if (CreateFunction == nullptr)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("No primitive scene proxy is registered under that name."));
			return nullptr;
		}
		return ((*CreateFunction)());
	}

	UINT32 RPrimitiveSceneProxyFactory::GetRegisteredNum()
	{
		return GetProxyFactoryTable().Num<UINT32>();
	}

} // namespace PigeonEngine
