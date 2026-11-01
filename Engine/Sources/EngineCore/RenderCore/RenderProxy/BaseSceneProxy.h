#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RScene;
	class RViewProxy;
	struct FShadowViewRequest;
	struct EViewDomainInfo;

	class RBaseSceneProxy : public EObjectBase
	{
	public:
		const Vector3&		GetWorldLocation()const;
		const Quaternion&	GetWorldRotation()const;
		const Vector3&		GetWorldScaling()const;
		const Matrix4x4&	GetLocalToWorldMatrix()const;
	public:
		void	SetProxyWorldTransform(const Vector3& InWorldLocation, const Quaternion& InWorldRotation, const Vector3& InWorldScaling);
	public:
		/*
		* Shadow views this proxy asks for, given a camera view. A light answers with one request
		* per view it needs - one per cascade, one for a spot, six for a point - and every other
		* proxy kind answers FALSE. The renderer does not know which light type it is holding: it
		* allocates a tile, asks BuildShadowViewDomain to fit the projection to the tile it got,
		* and records the depth pass.
		*/
		virtual BOOL32	CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests)const;
		/*
		* Builds the view and projection for one request at the resolution actually allocated.
		* OutAnchor/OutRotation are the view's world position and orientation, which is what the
		* ViewParams publish needs; the domain's viewport is filled by the renderer with the tile
		* rectangle, so the projection here must only depend on InResolution.
		*/
		virtual BOOL32	BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
			, const UINT32 InResolution, EViewDomainInfo& OutDomain
			, Vector3& OutAnchor, Quaternion& OutRotation)const;
	protected:
		Vector3		WorldLocation;
		Quaternion	WorldRotation;
		Vector3		WorldScaling;
		Matrix4x4	LocalToWorldMatrix;
	public:
		const RScene*	GetRenderScene()const;
		PE_INLINE void	SetRenderScene(const RScene* InScene);
	protected:
		const RScene*	Scene;

		RENDER_PROXY_CLASS_BODY(RBaseSceneProxy)

	};

};
