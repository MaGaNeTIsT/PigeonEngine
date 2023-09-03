/*
#include "../Headers/CCamera.h"
#include "../../EngineBase/Headers/CTimer.h"
#include "../../EngineBase/Headers/CInput.h"
#include "../../EngineBase/Headers/CManager.h"

const static CustomType::Vector2 _GStaticPointScale(0.5f, -0.5f);
const static CustomType::Vector2 _GStaticPointAdd(0.5f, 0.5f);

CCamera::CCamera(const BOOL32& active, const class CScene* scene) : CGameObject(active, scene)
{
	this->m_CameraInfo.Viewport = CustomStruct::CRenderViewport(CustomType::Vector4(0, 0, ENGINE_SCREEN_WIDTH, ENGINE_SCREEN_HEIGHT), CustomType::Vector2(0.f, 1.f));
	this->m_CameraInfo.Fov		= ENGINE_CAMERA_FOV;
	this->m_CameraInfo.Near		= ENGINE_CAMERA_NEAR;
	this->m_CameraInfo.Far		= ENGINE_CAMERA_FAR;

	this->m_CameraControlInfo.MoveSpeed = ENGINE_CAMERA_MOVE_SPEED;
	this->m_CameraControlInfo.LookSpeed = ENGINE_CAMERA_LOOK_SPEED * CustomType::CMath::GetDegToRad();
}
CCamera::~CCamera()
{
}
CustomStruct::CCullingFrustumInfo CCamera::PrepareTempFrustumInfo()const
{
	CustomStruct::CCullingFrustumInfo result;

	CustomType::Vector3 tempWorldPosition(this->GetWorldPosition());
	result.CameraForwardVec = this->GetForwardVector();

	{
		std::vector<CustomType::Vector3> tempPlane(this->GetCullingPlane());
		for (UINT32 i = 0u; i < 4u; i++)
		{
			result.CameraFrustumPlane[i] = tempPlane[i];
			result.CameraProjectPlane[i] = CustomType::Vector3::Dot(tempWorldPosition, -tempPlane[i]);
		}
	}

	{
		CustomType::Vector3 tempNear(tempWorldPosition + result.CameraForwardVec * this->m_CameraInfo.Near);
		CustomType::Vector3 tempFar(tempWorldPosition + result.CameraForwardVec * this->m_CameraInfo.Far);
		result.CameraProjectNear = CustomType::Vector3::Dot(tempNear, result.CameraForwardVec);
		result.CameraProjectFar = CustomType::Vector3::Dot(tempFar, -result.CameraForwardVec);
	}
	
	return result;
}
void CCamera::TransformPointFromWorldToView(const CustomType::Vector3& input, CustomType::Vector3& output)const
{
	output = this->GetViewMatrix().MultiplyPosition(input);
}
void CCamera::TransformPointFromViewToWorld(const CustomType::Vector3& input, CustomType::Vector3& output)const
{
	output = this->GetViewInverseMatrix().MultiplyPosition(input);
}
void CCamera::TransformWorldPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector2& output)const
{
	CustomType::Vector4 point(input.X(), input.Y(), input.Z(), 1.f);
	point = this->GetViewProjectionMatrix().MultiplyVector(point);
	FLOAT screenCoordX = CustomType::CMath::Clamp(point.X() / point.W(), -1.f, 1.f);
	FLOAT screenCoordY = CustomType::CMath::Clamp(point.Y() / point.W(), -1.f, 1.f);

	CustomType::Vector2 viewPortPointMul(this->m_CameraInfo.Viewport.Width, this->m_CameraInfo.Viewport.Height);
	CustomType::Vector2 viewPortPointAdd(this->m_CameraInfo.Viewport.TopLeftX, this->m_CameraInfo.Viewport.TopLeftY);

	CustomType::Vector2 result(screenCoordX, screenCoordY);
	if (point.Z() < 0.f)
	{
		result *= -1.f;
	}
	output = (result * _GStaticPointScale + _GStaticPointAdd) * viewPortPointMul + viewPortPointAdd;
}
BOOL32 CCamera::TransformWorldPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector3& output)const
{
	CustomType::Vector4 point(input.X(), input.Y(), input.Z(), 1.f);
	point = this->GetViewProjectionMatrix().MultiplyVector(point);
	point = point / point.W();
	if (point.X() < -1.f || point.X() > 1.f ||
		point.Y() < -1.f || point.Y() > 1.f ||
		point.Z() < 0.f || point.Z() > 1.f)
	{
		return FALSE;
	}

	const static CustomType::Vector3 _StaticPointScale(_GStaticPointScale.X(), _GStaticPointScale.Y(), 1.f);
	const static CustomType::Vector3 _StaticPointAdd(_GStaticPointAdd.X(), _GStaticPointAdd.Y(), 0.f);

	CustomType::Vector3 viewPortPointMul(this->m_CameraInfo.Viewport.Width, this->m_CameraInfo.Viewport.Height, 1.f);
	CustomType::Vector3 viewPortPointAdd(this->m_CameraInfo.Viewport.TopLeftX, this->m_CameraInfo.Viewport.TopLeftY, 0.f);

	CustomType::Vector3 result(point.GetXMFLOAT4());
	output = (result * _StaticPointScale + _StaticPointAdd) * viewPortPointMul + viewPortPointAdd;

	return TRUE;
}
void CCamera::TransformViewPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector2& output)const
{
	CustomType::Vector4 point(input.X(), input.Y(), input.Z(), 1.f);
	point = this->GetProjectionMatrix().MultiplyVector(point);
	FLOAT screenCoordX = CustomType::CMath::Clamp(point.X() / point.W(), -1.f, 1.f);
	FLOAT screenCoordY = CustomType::CMath::Clamp(point.Y() / point.W(), -1.f, 1.f);

	CustomType::Vector2 viewPortPointMul(this->m_CameraInfo.Viewport.Width, this->m_CameraInfo.Viewport.Height);
	CustomType::Vector2 viewPortPointAdd(this->m_CameraInfo.Viewport.TopLeftX, this->m_CameraInfo.Viewport.TopLeftY);

	CustomType::Vector2 result(screenCoordX, screenCoordY);
	if (point.Z() < 0.f)
	{
		result *= -1.f;
	}
	output = (result * _GStaticPointScale + _GStaticPointAdd) * viewPortPointMul + viewPortPointAdd;
}
BOOL32 CCamera::TransformViewPointToScreenCoord(const CustomType::Vector3& input, CustomType::Vector3& output)const
{
	CustomType::Vector4 point(input.X(), input.Y(), input.Z(), 1.f);
	point = this->GetProjectionMatrix().MultiplyVector(point);
	point = point / point.W();
	if (point.X() < -1.f || point.X() > 1.f ||
		point.Y() < -1.f || point.Y() > 1.f ||
		point.Z() < 0.f || point.Z() > 1.f)
	{
		return FALSE;
	}

	const static CustomType::Vector3 _StaticPointScale(_GStaticPointScale.X(), _GStaticPointScale.Y(), 1.f);
	const static CustomType::Vector3 _StaticPointAdd(_GStaticPointAdd.X(), _GStaticPointAdd.Y(), 0.f);

	CustomType::Vector3 viewPortPointMul(this->m_CameraInfo.Viewport.Width, this->m_CameraInfo.Viewport.Height, 1.f);
	CustomType::Vector3 viewPortPointAdd(this->m_CameraInfo.Viewport.TopLeftX, this->m_CameraInfo.Viewport.TopLeftY, 0.f);

	CustomType::Vector3 result(point.GetXMFLOAT4());
	output = (result * _StaticPointScale + _StaticPointAdd) * viewPortPointMul + viewPortPointAdd;

	return TRUE;
}
void CCamera::TransformScreenCoordToWorldPoint(const CustomType::Vector2& mousePos, CustomType::Vector3& output)const
{
	CustomType::Vector4 posSS(
		((mousePos.X() - this->m_CameraInfo.Viewport.TopLeftX) / this->m_CameraInfo.Viewport.Width - 0.5f) * 2.f,
		((mousePos.Y() - this->m_CameraInfo.Viewport.TopLeftY) / this->m_CameraInfo.Viewport.Height - 0.5f) * -2.f,
		1.f, 1.f);
	this->GetViewProjectionInverseMatrix().MultiplyVector(posSS);
	posSS /= posSS.W();
	output = CustomType::Vector3(posSS);
}
void CCamera::TransformScreenCoordToViewPoint(const CustomType::Vector2& mousePos, CustomType::Vector3& output)const
{
	CustomType::Vector4 posSS(
		((mousePos.X() - this->m_CameraInfo.Viewport.TopLeftX) / this->m_CameraInfo.Viewport.Width - 0.5f) * 2.f,
		((mousePos.Y() - this->m_CameraInfo.Viewport.TopLeftY) / this->m_CameraInfo.Viewport.Height - 0.5f) * -2.f,
		1.f, 1.f);
	this->GetProjectionInverseMatrix().MultiplyVector(posSS);
	posSS /= posSS.W();
	output = CustomType::Vector3(posSS);
}
std::vector<CustomType::Vector3> CCamera::GetCullingPlane()const
{
	CustomType::Quaternion wR(this->GetWorldRotation());
	std::vector<CustomType::Vector3> plane = {
		wR.MultiplyVector(this->m_FrustumInfo.Plane[0]),
		wR.MultiplyVector(this->m_FrustumInfo.Plane[1]),
		wR.MultiplyVector(this->m_FrustumInfo.Plane[2]),
		wR.MultiplyVector(this->m_FrustumInfo.Plane[3]) };
	return plane;
}
std::vector<CustomType::Vector3> CCamera::GetCullingPlanePoint()const
{
	CustomType::Vector3 wP(this->GetWorldPosition());
	CustomType::Quaternion wR(this->GetWorldRotation());
	std::vector<CustomType::Vector3> points = {
		wR.MultiplyVector(this->m_FrustumInfo.Point[0]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[1]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[2]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[3]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[4]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[5]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[6]),
		wR.MultiplyVector(this->m_FrustumInfo.Point[7]) };
	for (INT32 i = 0; i < 8; i++)
	{
		points[i] += wP;
	}
	return points;
}
CustomType::Vector4 CCamera::GetScreenToViewParameters(const CustomType::Vector2Int& finalViewport, const CustomType::Vector2Int& bufferSize)const
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
		for (INT32 i = 4; i < 8; i++)
		{
			plane.Point[i] = tempPosVec[i - 4];
		}
		FLOAT t = nearPlane / farPlane;
		for (INT32 i = 0; i < 4; i++)
		{
			plane.Point[i] = CustomType::Vector3::Lerp(0.f, tempPosVec[i], t);
		}
	}

	{
		for (INT32 i = 0; i < 4; i++)
		{
			tempPosVec[i].Normalize();
		}
		plane.Plane[0] = CustomType::Vector3::Cross(tempPosVec[0], tempPosVec[1]);
		plane.Plane[1] = CustomType::Vector3::Cross(tempPosVec[3], tempPosVec[2]);
		plane.Plane[2] = CustomType::Vector3::Cross(tempPosVec[2], tempPosVec[0]);
		plane.Plane[3] = CustomType::Vector3::Cross(tempPosVec[1], tempPosVec[3]);
		for (INT32 i = 0; i < 4; i++)
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
	this->m_ViewInvMatrix = CustomType::Matrix4x4(this->GetWorldPosition(), this->GetWorldRotation());
	this->m_ViewMatrix = this->m_ViewInvMatrix.Inverse();
}
void CCamera::ReCalculateViewProjectionMatrix()
{
	this->m_ViewProjectionMatrix = this->m_ViewMatrix * this->m_ProjectionMatrix;
	this->m_ViewProjectionInvMatrix = this->m_ViewProjectionMatrix.Inverse();
}
void CCamera::Init()
{
	this->AddNewTransform();

	CGameObject::Init();

	this->ReCalculateProjectionMatrix();
	this->ReCalculateViewMatrix();
	this->ReCalculateViewProjectionMatrix();
}
void CCamera::Uninit()
{
	CGameObject::Uninit();
}
void CCamera::Update()
{
	while (const auto e = CInput::Controller.ReadKey())
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
	}
	FLOAT deltaTime = static_cast<FLOAT>(CManager::GetGameTimer()->GetDeltaTime());
	{
		BOOL32 moveForward	= CInput::Controller.IsKeyPressed('W');
		BOOL32 moveBack		= CInput::Controller.IsKeyPressed('S');
		BOOL32 moveRight		= CInput::Controller.IsKeyPressed('D');
		BOOL32 moveLeft		= CInput::Controller.IsKeyPressed('A');
		BOOL32 moveUp			= CInput::Controller.IsKeyPressed('Q');
		BOOL32 moveDown		= CInput::Controller.IsKeyPressed('E');
		BOOL32 lookUp			= CInput::Controller.IsKeyPressed('I');
		BOOL32 lookDown		= CInput::Controller.IsKeyPressed('K');
		BOOL32 lookLeft		= CInput::Controller.IsKeyPressed('J');
		BOOL32 lookRight		= CInput::Controller.IsKeyPressed('L');
		CustomType::Vector3 worldPosition(this->GetWorldPosition());
		CustomType::Quaternion worldRotation(this->GetWorldRotation());
		if (moveForward || moveBack)
		{
			CustomType::Vector3 moveVector = this->GetForwardVector();
			if (moveForward)
			{
				worldPosition += moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
			}
			if (moveBack)
			{
				worldPosition += -moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
			}
		}
		if (moveRight || moveLeft)
		{
			CustomType::Vector3 moveVector = this->GetRightVector();
			moveVector = CustomType::Vector3(moveVector.X(), 0.f, moveVector.Z());
			moveVector.Normalize();
			if (moveRight)
			{
				worldPosition += moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
			}
			if (moveLeft)
			{
				worldPosition += -moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
			}
		}
		if (moveUp || moveDown)
		{
			CustomType::Vector3 moveVector = CustomType::Vector3(0.f, 1.f, 0.f);
			if (moveUp)
			{
				worldPosition += moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
			}
			if (moveDown)
			{
				worldPosition += -moveVector * (this->m_CameraControlInfo.MoveSpeed * deltaTime);
			}
		}
		if (lookUp || lookDown)
		{
			CustomType::Vector3 lookAxis = this->GetRightVector();
			lookAxis = CustomType::Vector3(lookAxis.X(), 0.f, lookAxis.Z());
			lookAxis.Normalize();
			if (lookUp)
			{
				CustomType::Quaternion lookRotation(lookAxis, -this->m_CameraControlInfo.LookSpeed * deltaTime);
				worldRotation = CustomType::Quaternion::MultiplyQuaternion(worldRotation, lookRotation);
			}
			if (lookDown)
			{
				CustomType::Quaternion lookRotation(lookAxis, this->m_CameraControlInfo.LookSpeed * deltaTime);
				worldRotation = CustomType::Quaternion::MultiplyQuaternion(worldRotation, lookRotation);
			}
		}
		if (lookLeft || lookRight)
		{
			CustomType::Vector3 lookAxis = CustomType::Vector3(0.f, 1.f, 0.f);
			if (lookLeft)
			{
				CustomType::Quaternion lookRotation(lookAxis, -this->m_CameraControlInfo.LookSpeed * deltaTime);
				worldRotation = CustomType::Quaternion::MultiplyQuaternion(worldRotation, lookRotation);
			}
			if (lookRight)
			{
				CustomType::Quaternion lookRotation(lookAxis, this->m_CameraControlInfo.LookSpeed * deltaTime);
				worldRotation = CustomType::Quaternion::MultiplyQuaternion(worldRotation, lookRotation);
			}
		}
		this->SetWorldPosition(worldPosition);
		this->SetWorldRotation(worldRotation);
	}

	BOOL32 bRightMouseButtonDown = CInput::Controller.IsRightMouseButtonDown();
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
			CustomType::Quaternion TargetRotation = this->GetWorldRotation();
			// Rotate around world vector (0,1,0) , whitch is world up vector
			TargetRotation = TargetRotation * TargetRotation.RotationAxis(CustomType::Vector3(0.f, 1.f, 0.f), delta->x * this->m_CameraControlInfo.LookSpeed * deltaTime * CustomType::CMath::GetDegToRad());
			// Rotate around object's right vector
			TargetRotation = TargetRotation * TargetRotation.RotationAxis(GetRightVector(), delta->y * this->m_CameraControlInfo.LookSpeed * deltaTime * CustomType::CMath::GetDegToRad());
			this->SetWorldRotation(TargetRotation);
		}
	}

	CGameObject::Update();

	this->ReCalculateViewMatrix();
	this->ReCalculateViewProjectionMatrix();
}
#ifdef _DEVELOPMENT_EDITOR
void CCamera::SelectedEditorUpdate()
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
#endif
*/