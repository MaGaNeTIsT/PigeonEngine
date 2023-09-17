#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderConfig/RenderConfig.h>
#include "BaseSceneProxy.h"
#include <RenderResource.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>

namespace PigeonEngine
{

	class RScene;

	class RViewProxy : public RBaseSceneProxy
	{
	public:
		using RVisibilityMapType	= TMap<ObjectIdentityType, BOOL32>;
	public:
		RViewProxy(const PCameraComponent* InComponent);
		void	SetupProxy(const BOOL32 InIsMainView, const ERenderViewMatrices& InMatrices, const ERenderViewParams& InParams);
	public:
		BOOL32						IsMainRenderView()const;
		RVisibilityMapType&			GetVisibilityMap();
		const RVisibilityMapType&	GetVisibilityMap()const;
		ECameraViewInfo&			GetViewInfo();
		const ECameraViewInfo&		GetViewInfo()const;
		EViewport&					GetRenderViewport();
		const EViewport&			GetRenderViewport()const;
		EViewMatrix&				GetViewMatrix();
		const EViewMatrix&			GetViewMatrix()const;
		EFrustum&					GetViewFrustum();
		const EFrustum&				GetViewFrustum()const;
	public:
		void	ResetVisibilityMap();
		void	UpdateViewSettings(const BOOL32 InIsMainView);
		void	UpdateMatrices(const ERenderViewMatrices& InMatrices);
		void	UpdateViewParams(const ERenderViewParams& InParams);
		void	UpdateRenderResource();
		void	BindRenderResource();
	protected:
		struct RViewCBufferData
		{
		public:
			RViewCBufferData() = default;
			RViewCBufferData(const RViewCBufferData& Other)
				: _ViewMatrix(Other._ViewMatrix)
				, _ViewInvMatrix(Other._ViewInvMatrix)
				, _ProjectionMatrix(Other._ProjectionMatrix)
				, _ProjectionInvMatrix(Other._ProjectionInvMatrix)
				, _ViewProjectionMatrix(Other._ViewProjectionMatrix)
				, _ViewProjectionInvMatrix(Other._ViewProjectionInvMatrix)
				, _TimeParams(Other._TimeParams)
				, _DepthMultiAdd(Other._DepthMultiAdd)
				, _ScreenToViewSpaceParams(Other._ScreenToViewSpaceParams)
				, _CameraViewportMinSizeAndInvBufferSize(Other._CameraViewportMinSizeAndInvBufferSize)
				, _CameraViewportSizeAndInvSize(Other._CameraViewportSizeAndInvSize)
				, _CameraViewportRect(Other._CameraViewportRect)
				, _CameraWorldPosition(Other._CameraWorldPosition)
			{
			}
			RViewCBufferData& operator=(const RViewCBufferData& Other)
			{
				_ViewMatrix = Other._ViewMatrix;
				_ViewInvMatrix = Other._ViewInvMatrix;
				_ProjectionMatrix = Other._ProjectionMatrix;
				_ProjectionInvMatrix = Other._ProjectionInvMatrix;
				_ViewProjectionMatrix = Other._ViewProjectionMatrix;
				_ViewProjectionInvMatrix = Other._ViewProjectionInvMatrix;
				_TimeParams = Other._TimeParams;
				_DepthMultiAdd = Other._DepthMultiAdd;
				_ScreenToViewSpaceParams = Other._ScreenToViewSpaceParams;
				_CameraViewportMinSizeAndInvBufferSize = Other._CameraViewportMinSizeAndInvBufferSize;
				_CameraViewportSizeAndInvSize = Other._CameraViewportSizeAndInvSize;
				_CameraViewportRect = Other._CameraViewportRect;
				_CameraWorldPosition = Other._CameraWorldPosition;
				return (*this);
			}
		public:
			DirectX::XMFLOAT4X4	_ViewMatrix;
			DirectX::XMFLOAT4X4	_ViewInvMatrix;
			DirectX::XMFLOAT4X4	_ProjectionMatrix;
			DirectX::XMFLOAT4X4	_ProjectionInvMatrix;
			DirectX::XMFLOAT4X4	_ViewProjectionMatrix;
			DirectX::XMFLOAT4X4	_ViewProjectionInvMatrix;
			DirectX::XMFLOAT4	_TimeParams;
			DirectX::XMFLOAT4	_DepthMultiAdd;
			DirectX::XMFLOAT4	_ScreenToViewSpaceParams;
			DirectX::XMFLOAT4	_CameraViewportMinSizeAndInvBufferSize;
			DirectX::XMFLOAT4	_CameraViewportSizeAndInvSize;
			DirectX::XMFLOAT4	_CameraViewportRect;
			DirectX::XMFLOAT4	_CameraWorldPosition;
		};
	public:
		const RBufferResource& GetViewCBuffer()const;
	protected:
		template<typename _TStructType>
		void CreateViewCBuffer();
		template<typename _TStructType>
		void UploadViewCBuffer(const _TStructType* InStruct);
		void BindViewCBuffer(const UINT32 InSlot);
	protected:
#if _EDITOR_ONLY
		UINT32						ViewCBufferSize;
#endif
		RBufferResource				ViewCBuffer;
	protected:
		BOOL32						IsMainView;
		RVisibilityMapType			VisibilityMap;
	protected:
		ECameraViewInfo				CameraViewInfo;
		EViewDomainInfo				ViewDomainInfo;
	protected:
		const PCameraComponent*		Component;

		RENDER_PROXY_CLASS_BODY(RViewProxy)

	};

};