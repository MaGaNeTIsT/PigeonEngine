#include <Navigation/NavigationManager.h>

#include <Editor/EditorLogManager.h>
#include <Navigation/NavigationProviderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ENavigationManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	// An invalid bound on either side means "unbounded", so a provider whose build did not
	// pin a volume still answers for any location.
	static BOOL32 BoundsContainLocation(const EBoundAABB& InBounds, const Vector3& InLocation)
	{
		if (!InBounds.IsValid)
		{
			return TRUE;
		}
		return ((InLocation.x >= InBounds.AABBMin.x) && (InLocation.x <= InBounds.AABBMax.x)
			&& (InLocation.y >= InBounds.AABBMin.y) && (InLocation.y <= InBounds.AABBMax.y)
			&& (InLocation.z >= InBounds.AABBMin.z) && (InLocation.z <= InBounds.AABBMax.z));
	}

	ENavigationManager::ENavigationManager()
	{
	}

	ENavigationManager::~ENavigationManager()
	{
		ShutDown();
	}

	void ENavigationManager::Initialize()
	{
	}

	void ENavigationManager::ShutDown()
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			delete Providers[Index].Provider;
			Providers[Index].Provider = nullptr;
		}
		Providers.Empty(0u);
		InvalidateDebugLines();
	}

	void ENavigationManager::RegisterProvider(const ObjectIdentityType& InOwnerId, ENavigationProviderInterface* InProvider)
	{
		if (!InProvider)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: registering a null provider was ignored.")));
			return;
		}

		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex >= 0)
		{
			// A rebuild replaces the provider. The old one is released here rather than at the
			// call site so ownership never depends on who built what.
			delete Providers[ExistingIndex].Provider;
			Providers[ExistingIndex].Provider = InProvider;
		}
		else
		{
			FNavigationProviderEntry Entry;
			Entry.OwnerId = InOwnerId;
			Entry.Provider = InProvider;
			Providers.Add(Entry);
		}
		InvalidateDebugLines();
	}

	void ENavigationManager::UnregisterProvider(const ObjectIdentityType& InOwnerId)
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex < 0)
		{
			return;
		}
		delete Providers[ExistingIndex].Provider;
		Providers[ExistingIndex].Provider = nullptr;
		Providers.RemoveAt(static_cast<UINT32>(ExistingIndex));
		InvalidateDebugLines();
	}

	void ENavigationManager::SetProviderDebugDrawEnabled(const ObjectIdentityType& InOwnerId, const BOOL32& InEnabled)
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		if (ExistingIndex < 0)
		{
			return;
		}
		if (Providers[ExistingIndex].bDebugDrawEnabled == InEnabled)
		{
			return;
		}
		Providers[ExistingIndex].bDebugDrawEnabled = InEnabled;
		InvalidateDebugLines();
	}

	ENavigationProviderInterface* ENavigationManager::GetProvider(const ObjectIdentityType& InOwnerId) const
	{
		const INT32 ExistingIndex = FindProviderIndex(InOwnerId);
		return (ExistingIndex >= 0) ? Providers[ExistingIndex].Provider : nullptr;
	}

	ENavigationProviderInterface* ENavigationManager::GetDefaultProvider() const
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			ENavigationProviderInterface* Provider = Providers[Index].Provider;
			if ((!!Provider) && (Provider->IsReady()))
			{
				return Provider;
			}
		}
		return nullptr;
	}

	ENavigationProviderInterface* ENavigationManager::FindProviderForLocation(const Vector3& InLocation) const
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			ENavigationProviderInterface* Provider = Providers[Index].Provider;
			if ((!!Provider) && (Provider->IsReady()) && (BoundsContainLocation(Provider->GetBounds(), InLocation)))
			{
				return Provider;
			}
		}
		// Outside every baked volume the default provider is still the best answer: it
		// projects the point onto its own mesh, which is what a caller with a stray start
		// position expects.
		return GetDefaultProvider();
	}

	UINT32 ENavigationManager::GetProviderNum() const
	{
		return Providers.Num<UINT32>();
	}

	BOOL32 ENavigationManager::FindPath(const Vector3& InStart, const Vector3& InEnd, ENavigationPath& OutPath) const
	{
		const ENavigationQueryFilter DefaultFilter;
		return FindPath(InStart, InEnd, DefaultFilter, OutPath);
	}

	BOOL32 ENavigationManager::FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InStart);
		if (!Provider)
		{
			return FALSE;
		}
		return Provider->FindPath(InStart, InEnd, InFilter, OutPath);
	}

	BOOL32 ENavigationManager::ProjectPoint(const Vector3& InLocation, Vector3& OutLocation) const
	{
		ENavigationProviderInterface* Provider = FindProviderForLocation(InLocation);
		if (!Provider)
		{
			return FALSE;
		}
		const ENavigationQueryFilter DefaultFilter;
		ENavigationProjectionResult Projection;
		if (!Provider->ProjectPoint(InLocation, DefaultFilter, Projection))
		{
			return FALSE;
		}
		OutLocation = Projection.Point;
		return TRUE;
	}

	void ENavigationManager::SetDebugDrawEnabled(const BOOL32& InEnabled)
	{
		if (bDebugDrawEnabled == InEnabled)
		{
			return;
		}
		bDebugDrawEnabled = InEnabled;
	}

	BOOL32 ENavigationManager::IsDebugDrawEnabled() const
	{
		return bDebugDrawEnabled;
	}

	void ENavigationManager::SetDebugDrawMode(ENavigationDebugModeType InMode)
	{
		if (DebugDrawMode == InMode)
		{
			return;
		}
		DebugDrawMode = InMode;
		InvalidateDebugLines();
	}

	ENavigationDebugModeType ENavigationManager::GetDebugDrawMode() const
	{
		return DebugDrawMode;
	}

	const TArray<Vector3>& ENavigationManager::GetDebugLineSegments()
	{
		if (bDebugLinesDirty)
		{
			DebugLineSegments.Empty();
			for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
			{
				ENavigationProviderInterface* Provider = Providers[Index].Provider;
				if ((!!Provider) && (Provider->IsReady()) && (Providers[Index].bDebugDrawEnabled))
				{
					Provider->CollectDebugLines(DebugDrawMode, DebugLineSegments);
				}
			}
			bDebugLinesDirty = FALSE;
		}
		return DebugLineSegments;
	}

	INT32 ENavigationManager::FindProviderIndex(const ObjectIdentityType& InOwnerId) const
	{
		for (UINT32 Index = 0u, Num = Providers.Num<UINT32>(); Index < Num; Index++)
		{
			if (Providers[Index].OwnerId == InOwnerId)
			{
				return static_cast<INT32>(Index);
			}
		}
		return -1;
	}

	void ENavigationManager::InvalidateDebugLines()
	{
		bDebugLinesDirty = TRUE;
		DebugLineSegments.Empty();
	}

}  // namespace PigeonEngine
