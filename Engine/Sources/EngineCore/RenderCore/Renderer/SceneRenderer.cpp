#include "SceneRenderer.h"
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/LightSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSceneRenderer, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RSceneRenderer::RSceneRenderer()
		: Scene(nullptr)
	{
	}
	RSceneRenderer::~RSceneRenderer()
	{
	}
	void RSceneRenderer::Initialize()
	{
	}
	void RSceneRenderer::ShutDown()
	{
	}
	void RSceneRenderer::InitNewFrame()
	{
		InitViews();
	}
	void RSceneRenderer::Render()
	{

	}
	void RSceneRenderer::InitViews()
	{
		// Update requires
		{
			RCommand& UpdateCommands = Scene->GetUpdateCommands();
			UpdateCommands.DoCommands();
			UpdateCommands.EmptyQueue();
		}

		// Add or remove requires
		{
			RCommand& AddOrRemoveCommands = Scene->GetAddOrRemoveCommands();
			AddOrRemoveCommands.DoCommands();
			AddOrRemoveCommands.EmptyQueue();
		}

		for (UINT32 ViewIndex = 0u, ViewNum = ViewProxies.Length(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if _EDITOR_ONLY
			Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that view proxy can not be null"), (!!ViewProxy));
			if (!ViewProxy)
			{
				continue;
			}
#endif
			const EFrustum& ViewFrustum = ViewProxy->GetViewFrustum();

			// CSM culling
			{
				RSceneProxyMapping<RDirectionalLightSceneProxy>& DirectionalLightSceneProxies = Scene->GetDirectionalLightSceneProxies();
				for (UINT32 DirectionalLightIndex = 0u, DirectionalLightNum = DirectionalLightSceneProxies.GetSceneProxyCount(); DirectionalLightIndex < DirectionalLightNum; DirectionalLightIndex++)
				{
					RDirectionalLightSceneProxy* LightProxy = DirectionalLightSceneProxies.SceneProxies[DirectionalLightIndex];
#if _EDITOR_ONLY
					Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that light proxy can not be null"), (!!LightProxy));
					if (!LightProxy)
					{
						continue;
					}
#endif
					LightProxy->GenerateViewInfo(ViewProxy);
					RDirectionalLightSceneProxy::RPerViewVisibilityMapType& VisibilityMaps = LightProxy->GetVisibilityMap();
					if (!(VisibilityMaps.ContainsKey(ViewProxy->GetUniqueID())))
					{
						VisibilityMaps.Add(ViewProxy->GetUniqueID(), TArray<RDirectionalLightSceneProxy::RVisibilityMapType>());
					}
					TArray<RDirectionalLightSceneProxy::RVisibilityMapType>& LayerVisibilityMap = VisibilityMaps[ViewProxy->GetUniqueID()];
					const TArray<EViewDomainInfo>& ViewDomainInfos = LightProxy->GetViewDomainInfos();
					Check((ENGINE_RENDER_CORE_ERROR), ("Check light view domain invalid."), ((ViewDomainInfos.Length() > 0u) && ((!(LightProxy->IsLightUseCascadeShadow())) || ((LightProxy->IsLightUseCascadeShadow()) && (!!(LightProxy->GetCascadeShadowData())) && ((LightProxy->GetCascadeShadowData()->Layers.Length()) == (ViewDomainInfos.Length()))))));
					for (UINT32 DomainIndex = 0u, DomainNum = ViewDomainInfos.Length(); DomainIndex < DomainNum; DomainIndex++)
					{
						if (DomainIndex >= LayerVisibilityMap.Length())
						{
							LayerVisibilityMap.Add(RDirectionalLightSceneProxy::RVisibilityMapType());
						}
						RDirectionalLightSceneProxy::RVisibilityMapType& VisibilityMap = LayerVisibilityMap[DomainIndex];
						const EViewDomainInfo& ViewDomainInfo = ViewDomainInfos[DomainIndex];
						OctreeCull(ViewDomainInfo.ViewFrustum, VisibilityMap);
						PrimitiveCull(ViewDomainInfo.ViewFrustum, VisibilityMap);
					}
				}
			}

			// Visible culling
			{
				RSceneRenderer::RVisibilityMapType& VisibilityMap = ViewProxy->GetVisibilityMap();
				OctreeCull(ViewFrustum, VisibilityMap);
				PrimitiveCull(ViewFrustum, VisibilityMap);
			}

		}
	}
	void RSceneRenderer::OctreeCull(const EFrustum& InViewFrustum, RSceneRenderer::RVisibilityMapType& InOutVisibilityMap)
	{
		ROctree& SceneOctree = Scene->GetSceneOctree();
		if (SceneOctree.GetPrimitiveNum() == 0u)
		{
			return;
		}

		const Vector3						TempAxis[3]			= { Vector3::XVector(), Vector3::YVector(), Vector3::ZVector() };
		const TArray<ROctreeElement>&		OctreeElements		= SceneOctree.GetElements();
		const TArray<ROctreeLayerInfo>&		OctreeLayerInfos	= SceneOctree.GetLayerInfos();

		INT32 TempDeep = 0;
		SceneOctree.BackwardRecursionNode(0, TempDeep,
			[&InOutVisibilityMap, &OctreeElements](ROctreeNode& InNode, INT32 InDeep)->INT32
			{
				Check((ENGINE_RENDER_CORE_ERROR), ("Check element index of octree node is valid failed."), ((InNode.ElementIndex >= 0) && (InNode.ElementIndex < OctreeElements.Length())));
				const ROctreeElement& OctreeElement = OctreeElements[InNode.ElementIndex];
				for (UINT32 PrimitiveIndex = 0u, PrimitiveNum = OctreeElement.Primitives.Length(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
				{
					const RPrimitiveSceneProxy* PrimitiveSceneProxy = OctreeElement.Primitives[PrimitiveIndex];
#if _EDITOR_ONLY
					Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive scene proxy is null."), (!!PrimitiveSceneProxy));
					if (!PrimitiveSceneProxy)
					{
						continue;
					}
#endif
					//TODO We can use primitive bounds for more precise culling results.
					//TODO May use C++17 function for map[]
					InOutVisibilityMap[PrimitiveSceneProxy->GetUniqueID()] = TRUE;
				}
				return (InDeep + 1);
			},
			[&TempAxis, &InViewFrustum, &OctreeLayerInfos](ROctreeNode& InNode, INT32 InDeep)->BOOL32
			{
				Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene octree deep with layer infos failed."), (InDeep < OctreeLayerInfos.Length()));
				const Vector3 NodeOrigin = InNode.Origin;
				const Vector3 NodeExtent = OctreeLayerInfos[InDeep].Extent;
				const FLOAT AxisExtent[3] = { NodeExtent.x, NodeExtent.x, NodeExtent.z };
				for (UINT32 i = 0u; i < 3u; i++)
				{
					const FLOAT NodeProjectOrigin = Vector3::Dot(TempAxis[i], NodeOrigin);
					if ((InViewFrustum.SeparateProjection[i].x > (NodeProjectOrigin + AxisExtent[i])) || (InViewFrustum.SeparateProjection[i].y < (NodeProjectOrigin - AxisExtent[i])))
					{
						return FALSE;
					}
				}
				return TRUE;
			});
	}
	void RSceneRenderer::PrimitiveCull(const EFrustum& InViewFrustum, RSceneRenderer::RVisibilityMapType& InOutVisibilityMap)
	{
		//Scene->GetSkeletalMeshSceneProxies;
	}

};