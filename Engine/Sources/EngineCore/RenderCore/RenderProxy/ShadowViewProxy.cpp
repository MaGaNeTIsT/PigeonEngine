#include "ShadowViewProxy.h"
#include "BaseSceneProxy.h"
#include <Base/RTTI/RTTIManager.h>

namespace PigeonEngine
{

	static void RegisterShadowViewProxyClassTypes()
	{
		RegisterClassType<RShadowViewProxy, RViewProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterShadowViewProxyClassTypes);

	RShadowViewProxy::RShadowViewProxy()
	{
	}
	RShadowViewProxy::RShadowViewProxy(const RShadowViewProxy& Other)
		: RViewProxy(Other)
		, Kind(Other.Kind)
		, LightProxy(Other.LightProxy)
		, ShadowSlot(Other.ShadowSlot)
		, CascadeIndex(Other.CascadeIndex)
		, CascadeCount(Other.CascadeCount)
		, ShadowViewKey(Other.ShadowViewKey)
		, ShadowDomainInfo(Other.ShadowDomainInfo)
		, bUsedThisFrame(FALSE)
	{
	}
	RShadowViewProxy::~RShadowViewProxy()
	{
	}
	void RShadowViewProxy::SetupShadowView(const EShadowViewKindType InKind, const RBaseSceneProxy* InLightProxy
		, const UINT32 InShadowSlot, const UINT32 InCascadeIndex, const UINT32 InCascadeCount
		, const EViewDomainInfo& InDomainInfo
		, const Vector3& InViewAnchor, const Quaternion& InViewRotation)
	{
		Kind			= InKind;
		LightProxy		= InLightProxy;
		ShadowSlot		= InShadowSlot;
		CascadeIndex	= InCascadeIndex;
		CascadeCount	= InCascadeCount;
		ShadowDomainInfo = InDomainInfo;

		// The anchor and rotation are what ViewParams publishes as ViewPosition/ViewDirection. A
		// shadow view has no camera component, so it cannot go through UpdateMatrices: that path
		// regenerates the frustum from a camera's world transform, and here the frustum was built
		// by the light from its own projection.
		SetProxyWorldTransform(InViewAnchor, InViewRotation, Vector3::One());
		IsMainView			= FALSE;
		ViewDomainInfo		= InDomainInfo;
		MarkViewParametersDirty();
		ResetVisibilityMap();
		bUsedThisFrame		= TRUE;
	}

} // namespace PigeonEngine
