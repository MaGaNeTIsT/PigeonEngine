#include "../../../../Entry/EngineMain.h"
#include "../Headers/CCamera.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../../EngineBase/Headers/CManager.h"

CCamera::CCamera()
{
	this->m_CameraInfo.Viewport = CustomStruct::CRenderViewport(CustomType::Vector4(0, 0, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT), CustomType::Vector2(0.f, 1.f));
	this->m_CameraInfo.Fov		= ENGINE_CAMERA_FOV;
	this->m_CameraInfo.Near		= ENGINE_CAMERA_NEAR;
	this->m_CameraInfo.Far		= ENGINE_CAMERA_FAR;

	this->m_CameraControlInfo.MoveSpeed = ENGINE_CAMERA_MOVE_SPEED;
	this->m_CameraControlInfo.LookSpeed = ENGINE_CAMERA_LOOK_SPEED * CustomType::CMath::GetDegToRad();

	this->m_Position.Reset();
	this->m_Rotation.Identity();
	this->m_Scale = 1.f;

	this->ReCalculateProjectionMatrix();
	this->ReCalculateViewMatrix();
	this->ReCalculateViewProjectionMatrix();
}
CCamera::CCamera(const CustomType::Vector3& position, const CustomType::Quaternion& rotation)
{
	this->m_CameraInfo.Viewport = CustomStruct::CRenderViewport(CustomType::Vector4(0, 0, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT), CustomType::Vector2(0.f, 1.f));
	this->m_CameraInfo.Fov		= ENGINE_CAMERA_FOV;
	this->m_CameraInfo.Near		= ENGINE_CAMERA_NEAR;
	this->m_CameraInfo.Far		= ENGINE_CAMERA_FAR;

	this->m_CameraControlInfo.MoveSpeed	= ENGINE_CAMERA_MOVE_SPEED;
	this->m_CameraControlInfo.LookSpeed	= ENGINE_CAMERA_LOOK_SPEED * CustomType::CMath::GetDegToRad();

	this->m_Position	= position;
	this->m_Rotation	= rotation;
	this->m_Scale		= 1.f;

	this->ReCalculateProjectionMatrix();
	this->ReCalculateViewMatrix();
	this->ReCalculateViewProjectionMatrix();
}
CCamera::CCamera(const CCamera& camera)
{
	this->m_Position				= camera.m_Position;
	this->m_Rotation				= camera.m_Rotation;
	this->m_Scale					= 1.f;

	this->m_CameraInfo				= camera.m_CameraInfo;
	this->m_CameraControlInfo		= camera.m_CameraControlInfo;
	this->m_FrustumInfo				= camera.m_FrustumInfo;

	this->m_ViewMatrix				= camera.m_ViewMatrix;
	this->m_ViewInvMatrix			= camera.m_ViewInvMatrix;
	this->m_ProjectionMatrix		= camera.m_ProjectionMatrix;
	this->m_ProjectionInvMatrix		= camera.m_ProjectionInvMatrix;
	this->m_ViewProjectionMatrix	= camera.m_ViewProjectionMatrix;
	this->m_ViewProjectionInvMatrix	= camera.m_ViewProjectionInvMatrix;
	this->m_ViewportMinSize			= camera.m_ViewportMinSize;
	this->m_ViewportSizeAndInvSize	= camera.m_ViewportSizeAndInvSize;
	this->m_DeviceZToViewZMulAdd	= camera.m_DeviceZToViewZMulAdd;
}
CCamera::~CCamera()
{
}
std::vector<CustomType::Vector3> CCamera::GetCullingPlane()
{
	CustomType::Quaternion wR(this->GetRotation());
	std::vector<CustomType::Vector3> plane = {
		wR.MultiplyVector(this->m_FrustumInfo.Plane[0]),
		wR.MultiplyVector(this->m_FrustumInfo.Plane[1]),
		wR.MultiplyVector(this->m_FrustumInfo.Plane[2]),
		wR.MultiplyVector(this->m_FrustumInfo.Plane[3]) };
	return plane;
}
std::vector<CustomType::Vector3> CCamera::GetCullingPlanePoint()
{
	CustomType::Vector3 wP(this->GetPosition());
	CustomType::Quaternion wR(this->GetRotation());
	std::vector<CustomType::Vector3> points = {
		wR.MultiplyVector(this->m_FrustumInfo.Point[0]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[1]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[2]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[3]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[4]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[5]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[6]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[7]) };
	for (INT i = 0; i < 8; i++)
	{
		points[i] += wP;
	}
	return points;
}
CustomType::Vector4 CCamera::GetScreenToViewParameters(const CustomType::Vector2Int& finalViewport, const CustomType::Vector2Int& bufferSize)
{
	FLOAT aspectRatio	= static_cast<FLOAT>(finalViewport.X()) / static_cast<FLOAT>(finalViewport.Y());
	FLOAT invTanHalfFov	= this->m_ProjectionMatrix.GetXMFLOAT4X4()._11;
	FLOAT invFovFixX	= 1.f / (invTanHalfFov);
	FLOAT invFovFixY	= 1.f / (aspectRatio * invTanHalfFov);
	FLOAT sizeX			= static_cast<FLOAT>(bufferSize.X());
	FLOAT sizeY			= static_cast<FLOAT>(bufferSize.Y());
	return CustomType::Vector4(
		sizeX * this->m_ViewportSizeAndInvSize.Z() * 2.f * invFovFixX,
		sizeY * this->m_ViewportSizeAndInvSize.W() * -2.f * invFovFixY,
		-((this->m_CameraInfo.Viewport.TopLeftX * this->m_ViewportSizeAndInvSize.Z() * 2.f * invFovFixX) + invFovFixX),
		(this->m_CameraInfo.Viewport.TopLeftY * this->m_ViewportSizeAndInvSize.W() * 2.f * invFovFixY) + invFovFixY);
}
void CCamera::ReCalculateFrustumPlane(CFrustumPlane& plane, const FLOAT& fovAngleY, const FLOAT& aspectRatio, const FLOAT& nearPlane, const FLOAT& farPlane)
{
	CustomType::Vector3 tempPosVec[4];
	{
		FLOAT sinHalfAngleY, cosHalfAngleY;
		CustomType::CMath::SinCos(sinHalfAngleY, cosHalfAngleY, (0.5f * fovAngleY) * CustomType::CMath::GetDegToRad());
		FLOAT halfHeight = (sinHalfAngleY / cosHalfAngleY) * farPlane;
		FLOAT halfWidth = aspectRatio * halfHeight;
		//
		//          Y
		//          ^
		//          |
		//          |
		//  0---------------1
		//  |               |
		//  |               |----->X
		//  |               |
		//  2---------------3
		//
		tempPosVec[0] = CustomType::Vector3(-halfWidth, halfHeight, farPlane);
		tempPosVec[1] = CustomType::Vector3(halfWidth, halfHeight, farPlane);
		tempPosVec[2] = CustomType::Vector3(-halfWidth, -halfHeight, farPlane);
		tempPosVec[3] = CustomType::Vector3(halfWidth, -halfHeight, farPlane);
	}

	{
		for (INT i = 4; i < 8; i++)
		{
			plane.Point[i] = tempPosVec[i - 4];
		}
		FLOAT t = nearPlane / farPlane;
		for (INT i = 0; i < 4; i++)
		{
			plane.Point[i] = CustomType::Vector3::Lerp(0.f, tempPosVec[i], t);
		}
	}

	{
		for (INT i = 0; i < 4; i++)
		{
			tempPosVec[i].Normalize();
		}
		plane.Plane[0] = CustomType::Vector3::Cross(tempPosVec[0], tempPosVec[1]);
		plane.Plane[1] = CustomType::Vector3::Cross(tempPosVec[3], tempPosVec[2]);
		plane.Plane[2] = CustomType::Vector3::Cross(tempPosVec[2], tempPosVec[0]);
		plane.Plane[3] = CustomType::Vector3::Cross(tempPosVec[1], tempPosVec[3]);
		for (INT i = 0; i < 4; i++)
		{
			plane.Plane[i].Normalize();
		}
	}
}
void CCamera::ReCalculateProjectionMatrix()
{
	FLOAT viewportW		= this->m_CameraInfo.Viewport.Width;
	FLOAT viewportH		= this->m_CameraInfo.Viewport.Height;
	FLOAT aspectRatio	= viewportW / viewportH;

	this->m_ProjectionMatrix = CustomType::Matrix4x4::PerspectiveMatrix(this->m_CameraInfo.Fov, aspectRatio, this->m_CameraInfo.Near, this->m_CameraInfo.Far);
	this->m_ProjectionInvMatrix = this->m_ProjectionMatrix.Inverse();

	FLOAT depthAdd = this->m_ProjectionMatrix.GetXMFLOAT4X4()._33;
	FLOAT depthMul = this->m_ProjectionMatrix.GetXMFLOAT4X4()._43;
	if (CustomType::CMath::Abs(depthMul) < 1e-6f)
		depthMul = 1e-6f;
	depthMul = 1.f / depthMul;
	depthAdd = -depthAdd * depthMul;
	this->m_DeviceZToViewZMulAdd = CustomType::Vector2(depthMul, depthAdd);

	this->m_ViewportSizeAndInvSize = CustomType::Vector4(viewportW, viewportH, 1.f / viewportW, 1.f / viewportH);
	this->m_ViewportMinSize = CustomType::Vector2(this->m_CameraInfo.Viewport.TopLeftX, this->m_CameraInfo.Viewport.TopLeftY);

	this->ReCalculateFrustumPlane(this->m_FrustumInfo, this->m_CameraInfo.Fov, aspectRatio, this->m_CameraInfo.Near, this->m_CameraInfo.Far);
}
void CCamera::ReCalculateViewMatrix()
{
	this->m_ViewInvMatrix = CustomType::Matrix4x4(this->m_Position, this->m_Rotation);
	this->m_ViewMatrix = this->m_ViewInvMatrix.Inverse();
}
void CCamera::ReCalculateViewProjectionMatrix()
{
	this->m_ViewProjectionMatrix = this->m_ViewMatrix * this->m_ProjectionMatrix;
	this->m_ViewProjectionInvMatrix = this->m_ViewProjectionMatrix.Inverse();
}
void CCamera::Init()
{
}
void CCamera::Uninit()
{

}
void CCamera::Update()
{
	{
		FLOAT moveSpeed = this->m_CameraControlInfo.MoveSpeed;
		FLOAT lookSpeed = this->m_CameraControlInfo.LookSpeed;
		ImGui::Begin("Camera properties");
		ImGui::InputFloat("Move speed", &moveSpeed);
		ImGui::InputFloat("Look speed", &lookSpeed);
		ImGui::End();
		this->m_CameraControlInfo.MoveSpeed = CustomType::CMath::Max(0.001f, moveSpeed);
		this->m_CameraControlInfo.LookSpeed = CustomType::CMath::Max(0.001f, lookSpeed);
	}

	/*while (const auto e = CInput::Controller.ReadKey())
	{
		if (!e->IsPress())
		{
			continue;
		}

		switch (e->GetCode())
		{

		case VK_PAUSE:
			if (CInput::Controller.IsCursorEnabled())
			{
				CInput::Controller.DisableCursor();
				CInput::Controller.EnableMouseRaw();
			}
			else
			{
				CInput::Controller.EnableCursor();
				CInput::Controller.DisableMouseRaw();
			}
			break;
		}
	}*/
	BOOL bRightMouseButtonDown = CInput::Controller.IsRightMouseButtonDown();
	BOOL moveForward	       = CInput::Controller.IsKeyPressed('W');
	BOOL moveBack		       = CInput::Controller.IsKeyPressed('S');
	BOOL moveRight		       = CInput::Controller.IsKeyPressed('D');
	BOOL moveLeft		       = CInput::Controller.IsKeyPressed('A');
	BOOL moveUp			       = CInput::Controller.IsKeyPressed('Q');
	BOOL moveDown		       = CInput::Controller.IsKeyPressed('E');
	BOOL lookUp			       = CInput::Controller.IsKeyPressed('I');
	BOOL lookDown		       = CInput::Controller.IsKeyPressed('K');
	BOOL lookLeft		       = CInput::Controller.IsKeyPressed('J');
	BOOL lookRight		       = CInput::Controller.IsKeyPressed('L');
	//BOOL lookRotClock		= CInput::GetKeyPress('O');
	//BOOL lookRotAntiClock	= CInput::GetKeyPress('U');
	FLOAT deltaTime = static_cast<FLOAT>(CManager::GetGameTimer()->GetDeltaTime());
	if (moveForward || moveBack)
	{
		CustomType::Vector3 moveVector = this->GetForwardVector();
		if (moveForward)
			this->m_Position += moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
		if (moveBack)
			this->m_Position += -moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
	}
	if (moveRight || moveLeft)
	{
		CustomType::Vector3 moveVector = this->GetRightVector();
		moveVector = CustomType::Vector3(moveVector.X(), 0.f, moveVector.Z());
		moveVector.Normalize();
		if (moveRight)
			this->m_Position += moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
		if (moveLeft)
			this->m_Position += -moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
	}
	if (moveUp || moveDown)
	{
		CustomType::Vector3 moveVector = CustomType::Vector3(0.f, 1.f, 0.f);
		if (moveUp)
			this->m_Position += moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
		if (moveDown)
			this->m_Position += -moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
	}
	if (lookUp || lookDown)
	{
		CustomType::Vector3 lookAxis = this->GetRightVector();
		lookAxis = CustomType::Vector3(lookAxis.X(), 0.f, lookAxis.Z());
		lookAxis.Normalize();
		if (lookUp)
		{
			CustomType::Quaternion lookRotation(lookAxis, -this->m_CameraControlInfo.LookSpeed * deltaTime);
			this->m_Rotation = CustomType::Quaternion::MultiplyQuaternion(this->m_Rotation, lookRotation);
		}
		if (lookDown)
		{
			CustomType::Quaternion lookRotation(lookAxis, this->m_CameraControlInfo.LookSpeed * deltaTime);
			this->m_Rotation = CustomType::Quaternion::MultiplyQuaternion(this->m_Rotation, lookRotation);
		}
	}
	if (lookLeft || lookRight)
	{
		CustomType::Vector3 lookAxis = CustomType::Vector3(0.f, 1.f, 0.f);
		if (lookLeft)
		{
			CustomType::Quaternion lookRotation(lookAxis, -this->m_CameraControlInfo.LookSpeed * deltaTime);
			this->m_Rotation = CustomType::Quaternion::MultiplyQuaternion(this->m_Rotation, lookRotation);
		}
		if (lookRight)
		{
			CustomType::Quaternion lookRotation(lookAxis, this->m_CameraControlInfo.LookSpeed * deltaTime);
			this->m_Rotation = CustomType::Quaternion::MultiplyQuaternion(this->m_Rotation, lookRotation);
		}
	}

	// Rotate camera while right mouse button down
	if (bRightMouseButtonDown)
	{
		CInput::Controller.DisableCursor();
		CInput::Controller.EnableMouseRaw();
	}
	else
	{
		CInput::Controller.EnableCursor();
		CInput::Controller.DisableMouseRaw();
	}
	
	if (!CInput::Controller.IsCursorEnabled())
	{
		while (const auto delta = CInput::ReadRawDelta())
		{
			CustomType::Quaternion TargetRotation = GetRotation();
			// Rotate around world vector (0,1,0) , whitch is world up vector
			TargetRotation = TargetRotation * TargetRotation.RotationAxis(CustomType::Vector3(0.f, 1.f, 0.f), delta->x * this->m_CameraControlInfo.LookSpeed * deltaTime * CustomType::CMath::GetDegToRad());
			// Rotate around object's right vector
			TargetRotation = TargetRotation * TargetRotation.RotationAxis(GetRightVector(), delta->y * this->m_CameraControlInfo.LookSpeed * deltaTime * CustomType::CMath::GetDegToRad());
			SetRotation(TargetRotation);
		}
	}

	this->ReCalculateViewMatrix();
	this->ReCalculateViewProjectionMatrix();
}