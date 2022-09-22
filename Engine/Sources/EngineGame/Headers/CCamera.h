#pragma once

#include "./CGameObject.h"

class CCamera : public CGameObject
{
public:
	struct CCameraInfo
	{
		CCameraInfo() { ZeroMemory(this, sizeof(*this)); }
		CustomType::Vector4	Viewport;
		CustomType::Vector2 Depth;
		FLOAT	Fov;
		FLOAT	Near;
		FLOAT	Far;
	};
	struct CCameraControlInfo
	{
		CCameraControlInfo() { ZeroMemory(this, sizeof(*this)); }
		FLOAT	MoveSpeed;
		FLOAT	LookSpeed;
	};
public:
	CustomType::Vector4		GetViewport() { return this->m_CameraInfo.Viewport; }
	CustomType::Vector2		GetViewportDepth() { return this->m_CameraInfo.Depth; }
	FLOAT					GetFov() { return this->m_CameraInfo.Fov; }
	FLOAT					GetNear() { return this->m_CameraInfo.Near; }
	FLOAT					GetFar() { return this->m_CameraInfo.Far; }
public:
	//The culling plane's normal of Camera' frustum with (top, down, left, right) order.
	std::vector<CustomType::Vector3> GetCullingPlane();
public:
	CustomType::Matrix4x4	GetViewMatrix() { return m_ViewMatrix; }
	CustomType::Matrix4x4	GetViewInverseMatrix() { return m_ViewInvMatrix; }
	CustomType::Matrix4x4	GetProjectionMatrix() { return m_ProjectionMatrix; }
	CustomType::Matrix4x4	GetProjectionInverseMatrix() { return m_ProjectionInvMatrix; }
	CustomType::Matrix4x4	GetViewProjectionMatrix() { return m_ViewProjectionMatrix; }
	CustomType::Matrix4x4	GetViewProjectionInverseMatrix() { return m_ViewProjectionInvMatrix; }
public:
	CustomType::Vector2		GetViewportMinSize() { return m_ViewportMinSize; }
	CustomType::Vector4		GetViewportSizeAndInvSize() { return m_ViewportSizeAndInvSize; }
	CustomType::Vector2		GetDeviceZToViewZMulAdd() { return m_DeviceZToViewZMulAdd; }
	CustomType::Vector4		GetScreenToViewParameters(const CustomType::Vector2Int& finalViewport, const CustomType::Vector2Int& bufferSize);
protected:
	//Calculate plane's normal of Camera' frustum with (top, down, left, right) order.
	void			ReCalculateFrustumPlane(std::vector<CustomType::Vector3>& plane, const FLOAT& fovAngleY, const FLOAT& aspectRatio, const FLOAT& farPlane);
	void			ReCalculateProjectionMatrix();
	void			ReCalculateViewMatrix();
	void			ReCalculateViewProjectionMatrix();
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;

public:
	CCamera();
	CCamera(const CustomType::Vector3& position, const CustomType::Quaternion& rotation);
	CCamera(const CCamera& camera);
	virtual ~CCamera();
protected:

	CCameraInfo							m_CameraInfo;
	CCameraControlInfo					m_CameraControlInfo;
	std::vector<CustomType::Vector3>	m_FrustumPlane;
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