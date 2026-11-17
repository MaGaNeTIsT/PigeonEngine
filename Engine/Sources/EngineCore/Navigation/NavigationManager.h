#pragma once

#include <Base/DataStructure/ObjectBase.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>

namespace PigeonEngine
{

	class ENavigationProviderInterface;

	/*
	* Runtime owner of every navigation provider in the process.
	*
	* A provider is registered by the component that built it (one provider per component,
	* keyed by the component's unique id), and gameplay asks this manager instead of holding a
	* provider pointer. That keeps the queries backend agnostic: the agent component calls
	* FindPath and never learns whether the answer came from Detour or from a future grid
	* backend.
	*
	* Thread affinity: game thread only, same as the world it describes. Bakes are
	* synchronous for now, so a provider is published and ready in the frame that built it.
	*/
	class ENavigationManager final : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ENavigationManager)
	public:
		void	Initialize() override;
		void	ShutDown() override;

	public:
		// Ownership of InProvider transfers to the manager. Registering twice for one owner
		// replaces the provider, which is what a rebuild does.
		void	RegisterProvider(const ObjectIdentityType& InOwnerId, ENavigationProviderInterface* InProvider);
		void	UnregisterProvider(const ObjectIdentityType& InOwnerId);
		// Per provider overlay switch. A level can hold several navigation volumes and only
		// the one under construction needs to be drawn.
		void	SetProviderDebugDrawEnabled(const ObjectIdentityType& InOwnerId, const BOOL32& InEnabled);
		PE_NODISCARD ENavigationProviderInterface* GetProvider(const ObjectIdentityType& InOwnerId) const;
		// First provider that finished a build. Null while nothing has been baked yet.
		PE_NODISCARD ENavigationProviderInterface* GetDefaultProvider() const;
		// Provider whose bounds contain InLocation, or the default one when none does.
		PE_NODISCARD ENavigationProviderInterface* FindProviderForLocation(const Vector3& InLocation) const;
		PE_NODISCARD UINT32 GetProviderNum() const;

	public:
		// Shortcuts used by gameplay: they resolve a provider and answer with engine types.
		// Both return FALSE when no provider is ready; OutPath keeps its previous contents.
		BOOL32	FindPath(const Vector3& InStart, const Vector3& InEnd, ENavigationPath& OutPath) const;
		BOOL32	FindPath(const Vector3& InStart, const Vector3& InEnd, const ENavigationQueryFilter& InFilter, ENavigationPath& OutPath) const;
		BOOL32	ProjectPoint(const Vector3& InLocation, Vector3& OutLocation) const;

	public:
		void						SetDebugDrawEnabled(const BOOL32& InEnabled);
		PE_NODISCARD BOOL32			IsDebugDrawEnabled() const;
		void						SetDebugDrawMode(ENavigationDebugModeType InMode);
		PE_NODISCARD ENavigationDebugModeType	GetDebugDrawMode() const;
		/*
		* Cached line segments, two entries per segment. The list is rebuilt on the first
		* request after a provider changed or the mode changed: walking every polygon each
		* frame would cost more than drawing them.
		*/
		const TArray<Vector3>&		GetDebugLineSegments();

	private:
		struct FNavigationProviderEntry
		{
			ObjectIdentityType			OwnerId = 0;
			ENavigationProviderInterface*	Provider = nullptr;
			BOOL32						bDebugDrawEnabled = TRUE;
		};
		PE_NODISCARD INT32	FindProviderIndex(const ObjectIdentityType& InOwnerId) const;
		void				InvalidateDebugLines();

	private:
		TArray<FNavigationProviderEntry>	Providers;
		BOOL32								bDebugDrawEnabled = TRUE;
		ENavigationDebugModeType			DebugDrawMode = ENavigationDebugModeType::NAVIGATION_DEBUG_BOUNDARY;
		TArray<Vector3>						DebugLineSegments;
		BOOL32								bDebugLinesDirty = TRUE;
	};

}  // namespace PigeonEngine
