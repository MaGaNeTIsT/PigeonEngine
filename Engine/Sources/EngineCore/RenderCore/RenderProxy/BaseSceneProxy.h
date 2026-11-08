#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RScene;
	class RViewProxy;
	struct FShadowViewRequest;
	struct EViewDomainInfo;
	struct EBoundAABB;

	class RBaseSceneProxy : public EObjectBase
	{
	public:
		const Vector3&		GetWorldLocation() const;
		const Quaternion&	GetWorldRotation() const;
		const Vector3&		GetWorldScaling() const;
		const Matrix4x4&	GetLocalToWorldMatrix() const;
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
		virtual BOOL32	CollectShadowViews(const RViewProxy& InView, TArray<FShadowViewRequest>& OutRequests) const;
		/*
		* Builds the view and projection for one request at the resolution actually allocated.
		* OutAnchor/OutRotation are the view's world position and orientation, which is what the
		* ViewParams publish needs; the domain's viewport is filled by the renderer with the tile
		* rectangle.
		*
		* InCasterBounds is the world space union of every primitive the frame can draw as a
		* caster, and InAreCasterBoundsComplete says whether that union covers them all. An
		* orthographic (directional) light uses it for the depth range its cascade must cover: an
		* occluder anywhere between the light and the receiver shadows it, so the slice alone
		* does not bound the casters. A perspective light has its own shape and ignores it.
		*/
		virtual BOOL32	BuildShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
			, const UINT32 InResolution, const EBoundAABB& InCasterBounds, const BOOL32 InAreCasterBoundsComplete
			, EViewDomainInfo& OutDomain, Vector3& OutAnchor, Quaternion& OutRotation) const;
		/*
		* Re-fits an orthographic shadow view's projection to the geometry its cull actually
		* kept. A directional cascade implements it: an orthographic light preserves an
		* occluder's light space x and y in the shadow it casts, so a box around the drawn
		* casters also contains every shadow they can cast, and fitting to it spends the tile's
		* texels on geometry rather than on the empty corners of the slice fitted sphere.
		*
		* FALSE keeps the domain BuildShadowViewDomain produced. That is the answer for a
		* perspective view (spot, point face), for an empty or incomplete union, and for every
		* proxy that is not a light.
		*/
		virtual BOOL32	RefitShadowViewDomain(const RViewProxy& InView, const FShadowViewRequest& InRequest
			, const UINT32 InResolution, const EBoundAABB& InDrawnBounds
			, EViewDomainInfo& InOutDomain) const;
	protected:
		Vector3		WorldLocation;
		Quaternion	WorldRotation;
		Vector3		WorldScaling;
		Matrix4x4	LocalToWorldMatrix;
	public:
		const RScene*	GetRenderScene() const;
		// Defined in the cpp, so it is not declared inline: an inline declaration whose only
		// definition lives in another translation unit emits no symbol for its callers.
		void			SetRenderScene(const RScene* InScene);
	protected:
		const RScene*	Scene;

		RENDER_PROXY_CLASS_BODY(RBaseSceneProxy)

	};

};
