#pragma once

#include "../../../../Entry/EngineMain.h"
#include "./CGameStructCommon.h"
#include "./CGameObject.h"

class CCamera : public CGameObject
{
public:
	struct CCameraInfo
	{
		CCameraInfo() { ZeroMemory(this, sizeof(*this)); }
		CustomStruct::CRenderViewport	Viewport;
		FLOAT							Fov;
		FLOAT							Near;
		FLOAT							Far;
	};
	struct CCameraControlInfo
	{
		CCameraControlInfo() { ZeroMemory(this, sizeof(*this)); }
		FLOAT	MoveSpeed;
		FLOAT	LookSpeed;
	};
	struct CFrustumPlane
	{
		CFrustumPlane() { ZeroMemory(this, sizeof(*this)); }
		CustomType::Vector3 Plane[4];
		CustomType::Vector3 Point[8];
	};
public:
	CustomStruct::CRenderViewport		GetViewport()const { return this->m_CameraInfo.Viewport; }
	FLOAT								GetFov()const { return this->m_CameraInfo.Fov; }
	FLOAT								GetNear()const { return this->m_CameraInfo.Near; }
	FLOAT								GetFar()const { return this->m_CameraInfo.Far; }
	CustomStruct::CCullingFrustumInfo	PrepareTempFrustumInfo()const;
public:
	void	TransformPointFromWorldToView(const CustomType::Vector3& input, CustomType::Vector3& output)const;
	void	TransformPointFromViewToWorld(const CustomType::Vector3& input, CustomType::Vector3& output)const;

	/*
	* Return value : if return false, the point is out of screen coord, else in screen.
	* Params input : specific space position of the point.
	* Params output : [0.f, screen max] range of screen coord which has anchor(zero point) in left top of RECT.
	*/
	void	TransformWorldPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector2& output)const;
	BOOL	TransformWorldPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector3& output)const;
	void	TransformViewPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector2& output)const;
	BOOL	TransformViewPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector3& output)const;

	/*
	* Params input X Y : screen space position of the point (range : [0.f, screen max]).
	* Params output : output space direction from camera to point. WARNING! output direction is NOT normalized.
	*/
	void	TransformScreenCoordToWorldPoint(const CustomType::Vector2& mousePos, CustomType::Vector3& output)const;
	void	TransformScreenCoordToViewPoint(const CustomType::Vector2& mousePos, CustomType::Vector3& output)const;
public:
	//The culling plane's normals of Camera' frustum with (top, down, left, right) order.
	std::vector<CustomType::Vector3> GetCullingPlane()const;
	//The culling plane's points of Camera' frustum with (0~3 near plane, 4~7 far plane) order.
	std::vector<CustomType::Vector3> GetCullingPlanePoint()const;
public:
	CustomType::Matrix4x4	GetViewMatrix()const { return m_ViewMatrix; }
	CustomType::Matrix4x4	GetViewInverseMatrix()const { return m_ViewInvMatrix; }
	CustomType::Matrix4x4	GetProjectionMatrix()const { return m_ProjectionMatrix; }
	CustomType::Matrix4x4	GetProjectionInverseMatrix()const { return m_ProjectionInvMatrix; }
	CustomType::Matrix4x4	GetViewProjectionMatrix()const { return m_ViewProjectionMatrix; }
	CustomType::Matrix4x4	GetViewProjectionInverseMatrix()const { return m_ViewProjectionInvMatrix; }
public:
	CustomType::Vector2		GetViewportMinSize()const { return m_ViewportMinSize; }
	CustomType::Vector4		GetViewportSizeAndInvSize()const { return m_ViewportSizeAndInvSize; }
	CustomType::Vector2		GetDeviceZToViewZMulAdd()const { return m_DeviceZToViewZMulAdd; }
	CustomType::Vector4		GetScreenToViewParameters(const CustomType::Vector2Int& finalViewport, const CustomType::Vector2Int& bufferSize)const;
protected:
	//Calculate plane's normal of Camera' frustum with (top, down, left, right) order.
	void			ReCalculateFrustumPlane(CFrustumPlane& plane, const FLOAT& fovAngleY, const FLOAT& aspectRatio, const FLOAT& nearPlane, const FLOAT& farPlane);
	void			ReCalculateProjectionMatrix();
	void			ReCalculateViewMatrix();
	void			ReCalculateViewProjectionMatrix();
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
public:
	CCamera();
	virtual ~CCamera();
protected:
	CCameraInfo							m_CameraInfo;
	CCameraControlInfo					m_CameraControlInfo;
	CFrustumPlane						m_FrustumInfo;
	CustomType::Matrix4x4				m_ViewMatrix;
	CustomType::Matrix4x4				m_ViewInvMatrix;
	CustomType::Matrix4x4				m_ProjectionMatrix;
	CustomType::Matrix4x4				m_ProjectionInvMatrix;
	CustomType::Matrix4x4				m_ViewProjectionMatrix;
	CustomType::Matrix4x4				m_ViewProjectionInvMatrix;
	CustomType::Vector2					m_ViewportMinSize;
	CustomType::Vector4					m_ViewportSizeAndInvSize;
	CustomType::Vector2					m_DeviceZToViewZMulAdd;
};