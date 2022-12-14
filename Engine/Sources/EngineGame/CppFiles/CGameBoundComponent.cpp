#include "../Headers/CGameBoundComponent.h"
#include "../Headers/CGameObject.h"
#include "../Headers/CCamera.h"

const static CustomType::Vector3 _GStaticBoxVector[8] = {
	CustomType::Vector3(-1.f, -1.f, -1.f),
	CustomType::Vector3(1.f,  1.f,  1.f),
	CustomType::Vector3(-1.f, -1.f,  1.f),
	CustomType::Vector3(1.f,  1.f, -1.f),
	CustomType::Vector3(1.f, -1.f,  1.f),
	CustomType::Vector3(-1.f,  1.f, -1.f),
	CustomType::Vector3(1.f, -1.f, -1.f),
	CustomType::Vector3(-1.f,  1.f,  1.f) };

CGameBoundBaseComponent::CGameBoundBaseComponent() : CBaseComponent(TRUE, FALSE, FALSE)
{

}
CGameBoundBaseComponent::~CGameBoundBaseComponent()
{

}
void CGameBoundBaseComponent::Init()
{

}
void CGameBoundBaseComponent::Uninit()
{

}
#ifdef _DEVELOPMENT_EDITOR
void CGameBoundBaseComponent::SelectedEditorUpdate()
{

}
#endif



CGameBoundSphereComponent::CGameBoundSphereComponent()
{
	this->m_Anchor = CustomType::Vector3::Zero();
	this->m_Radius = CustomStruct::CGameBaseSetting::GameBoundMinimum;
}
CGameBoundSphereComponent::CGameBoundSphereComponent(const CustomType::Vector3& anchor, const FLOAT& radius)
{
	this->m_Anchor = anchor;
	this->m_Radius = radius;
}
CGameBoundSphereComponent::~CGameBoundSphereComponent()
{

}
CustomType::Vector3 CGameBoundSphereComponent::GetWorldAnchor()const
{
	if (this->m_GameObject != NULL)
	{
		CustomType::Vector3 result(this->m_GameObject->GetWorldPosition() + this->m_Anchor);
		return result;
	}
	return (this->m_Anchor);
}
const CustomType::Vector3& CGameBoundSphereComponent::GetLocalAnchor()const
{
	return (this->m_Anchor);
}
const FLOAT& CGameBoundSphereComponent::GetRadius()const
{
	return (this->m_Radius);
}
void CGameBoundSphereComponent::SetLocalAnchorRadius(const CustomType::Vector3& p, const FLOAT& r)
{
	this->m_Anchor = p;
	this->m_Radius = r;
}
void CGameBoundSphereComponent::SetLocalAnchor(const CustomType::Vector3& v)
{
	this->m_Anchor = v;
}
void CGameBoundSphereComponent::SetRadius(const FLOAT& v)
{
	this->m_Radius = v;
}
BOOL CGameBoundSphereComponent::InsertCameraFrustum(const CustomStruct::CCullingFrustumInfo& cullingFrustum)const
{
	CustomType::Vector3 sphereAnchor(this->GetWorldAnchor());
	FLOAT sphereRadius = this->GetRadius();

	FLOAT dotTerm = CustomType::Vector3::Dot(sphereAnchor, cullingFrustum.CameraForwardVec);
	if ((dotTerm + sphereRadius) < cullingFrustum.CameraProjectNear || (-dotTerm + sphereRadius) < cullingFrustum.CameraProjectFar)
	{
		return FALSE;
	}

	for (UINT i = 0u; i < 4u; i++)
	{
		dotTerm = CustomType::Vector3::Dot(sphereAnchor, cullingFrustum.CameraFrustumPlane[i]);
		if ((dotTerm + sphereRadius) < cullingFrustum.CameraProjectPlane[i])
		{
			return FALSE;
		}
	}

	return TRUE;
}
CustomStruct::CRect CGameBoundSphereComponent::GetScreenCoordRect(const CCamera* camera)const
{
	CustomType::Vector3 sphereLocalAnchor(this->GetLocalAnchor());
	CustomType::Vector3 sphereRadius(this->GetRadius());

	const CGameObject* gameObject = this->GetGameObject();
	CustomType::Quaternion sphereR(CustomType::Quaternion::Identity());
	CustomType::Vector3 sphereW(CustomType::Vector3::Zero());
	if (gameObject != NULL)
	{
		sphereW = gameObject->GetWorldPosition();
		sphereR = gameObject->GetWorldRotation();
	}

	FLOAT screenMin[2] = { ENGINE_FLOAT32_MAX, ENGINE_FLOAT32_MAX }, screenMax[2] = { 0.f, 0.f };
	auto checkMinMax = [&screenMin, &screenMax](const CustomType::Vector2& input) {
		screenMin[0] = CustomType::CMath::Min(input.X(), screenMin[0]);
		screenMin[1] = CustomType::CMath::Min(input.Y(), screenMin[1]);
		screenMax[0] = CustomType::CMath::Max(input.X(), screenMax[0]);
		screenMax[1] = CustomType::CMath::Max(input.Y(), screenMax[1]); };
	auto checkScreenCoordMinMax = [&](const UINT& index, CustomType::Vector2& output) {
		CustomType::Vector3 tempPoint(sphereLocalAnchor + sphereRadius * _GStaticBoxVector[index]);
		if (gameObject != NULL)
		{
			tempPoint = sphereR.MultiplyVector(tempPoint) + sphereW;
		}
		camera->TransformWorldPointToScreenCoord(tempPoint, output); };
	{
		CustomType::Vector2 tempSphereBoundPoint;
		for (UINT i = 0u; i < 8u; i++)
		{
			checkScreenCoordMinMax(i, tempSphereBoundPoint);
			checkMinMax(tempSphereBoundPoint);
		}
	}

	return (CustomStruct::CRect(screenMin[0], screenMin[1], screenMax[0], screenMax[1]));
}
#ifdef _DEVELOPMENT_EDITOR
BOOL CGameBoundSphereComponent::SelectedInEditorScreen(const CustomType::Vector2& mousePos, const CCamera* camera, const CustomStruct::CCullingFrustumInfo& cullingFrustum, CustomType::Vector2& screenAnchor, FLOAT& viewDepth)const
{
	if (!InsertCameraFrustum(cullingFrustum))
	{
		return FALSE;
	}

	{
		CustomType::Vector3 tempStart(camera->GetWorldPosition());
		CustomType::Vector3 tempEnd;
		camera->TransformScreenCoordToWorldPoint(mousePos, tempEnd);
		if (RayCast(tempStart, tempEnd))
		{
			CustomType::Vector3 sphereAnchorWS(this->GetWorldAnchor());
			CustomType::Vector3 sphereAnchorVS;
			camera->TransformPointFromWorldToView(sphereAnchorWS, sphereAnchorVS);
			viewDepth = sphereAnchorVS.Z();
			camera->TransformWorldPointToScreenCoord(sphereAnchorWS, screenAnchor);
			return TRUE;
		}
	}

	return FALSE;
}
#endif
BOOL CGameBoundSphereComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd)const
{
	CustomType::Vector3 sphereLocalAnchor(this->GetLocalAnchor());
	FLOAT sphereRadius = this->GetRadius();
	FLOAT sphereRadiusSq = sphereRadius * sphereRadius;

	CustomType::Vector3 pointSphereC(sphereLocalAnchor - rayStart);
	FLOAT pointSphereCLenSq = pointSphereC.LengthSquare();

	if (pointSphereCLenSq <= sphereRadiusSq)
	{
		return TRUE;
	}

	CustomType::Vector3 rayDir(rayEnd); rayDir -= rayStart;
	FLOAT rayDirLenSq = rayDir.LengthSquare();
	FLOAT rayDirLen = CustomType::CMath::Sqrt(rayDirLenSq);

	if (rayDirLen < 1e-3f)
	{
		return FALSE;
	}

	rayDir /= rayDirLen;
	FLOAT projectLen = CustomType::Vector3::Dot(pointSphereC, rayDir);

	if (projectLen <= 0.f)
	{
		return FALSE;
	}

	FLOAT projectLenSq = projectLen * projectLen;

	if (projectLenSq > rayDirLenSq)
	{
		return FALSE;
	}

	FLOAT verticalLenSq = pointSphereCLenSq - projectLenSq;

	if (verticalLenSq <= sphereRadiusSq)
	{
		FLOAT inSphereLenSq = sphereRadiusSq - verticalLenSq;
		FLOAT inSphereLen = CustomType::CMath::Sqrt(inSphereLenSq);

		FLOAT t = projectLen - inSphereLen;
		if (t <= rayDirLen)
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL CGameBoundSphereComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen)const
{
	CustomType::Vector3 sphereLocalAnchor(this->GetLocalAnchor());
	FLOAT sphereRadius = this->GetRadius();
	FLOAT sphereRadiusSq = sphereRadius * sphereRadius;

	CustomType::Vector3 pointSphereC(sphereLocalAnchor - rayStart);
	FLOAT pointSphereCLenSq = pointSphereC.LengthSquare();

	if (pointSphereCLenSq <= sphereRadiusSq)
	{
		return TRUE;
	}

	if (rayLen < 1e-3f)
	{
		return FALSE;
	}

	FLOAT projectLen = CustomType::Vector3::Dot(pointSphereC, rayDir);

	if (projectLen <= 0.f)
	{
		return FALSE;
	}

	FLOAT rayDirLenSq = rayLen * rayLen;
	FLOAT projectLenSq = projectLen * projectLen;

	if (projectLenSq > rayDirLenSq)
	{
		return FALSE;
	}

	FLOAT verticalLenSq = pointSphereCLenSq - projectLenSq;

	if (verticalLenSq <= sphereRadiusSq)
	{
		FLOAT inSphereLenSq = sphereRadiusSq - verticalLenSq;
		FLOAT inSphereLen = CustomType::CMath::Sqrt(inSphereLenSq);

		FLOAT t = projectLen - inSphereLen;
		if (t <= rayLen)
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL CGameBoundSphereComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd, std::vector<FLOAT>& intersectPoints)const
{
	if (intersectPoints.size() > 0)
	{
		intersectPoints.clear();
	}

	CustomType::Vector3 sphereLocalAnchor(this->GetLocalAnchor());
	FLOAT sphereRadius = this->GetRadius();
	FLOAT sphereRadiusSq = sphereRadius * sphereRadius;

	CustomType::Vector3 rayDir(rayEnd); rayDir -= rayStart;
	FLOAT rayDirLenSq = rayDir.LengthSquare();

	CustomType::Vector3 pointSphereC(sphereLocalAnchor - rayStart);
	FLOAT pointSphereCLenSq = pointSphereC.LengthSquare();

	if (pointSphereCLenSq <= sphereRadiusSq)
	{
		intersectPoints.push_back(0.f);

		FLOAT rayDirLen = CustomType::CMath::Sqrt(rayDirLenSq);
		if (rayDirLen < 1e-3f)
		{
			return TRUE;
		}
		rayDir /= rayDirLen;
		FLOAT projectLen = CustomType::Vector3::Dot(pointSphereC, rayDir);
		FLOAT projectLenSq = projectLen * projectLen;
		FLOAT verticalLenSq = pointSphereCLenSq - projectLenSq;

		intersectPoints.push_back(CustomType::CMath::Sqrt(sphereRadiusSq - verticalLenSq) + projectLen);

		return TRUE;
	}

	FLOAT rayDirLen = CustomType::CMath::Sqrt(rayDirLenSq);
	if (rayDirLen < 1e-3f)
	{
		return FALSE;
	}

	rayDir /= rayDirLen;
	FLOAT projectLen = CustomType::Vector3::Dot(pointSphereC, rayDir);

	if (projectLen <= 0.f)
	{
		return FALSE;
	}

	FLOAT projectLenSq = projectLen * projectLen;

	if (projectLenSq > rayDirLenSq)
	{
		return FALSE;
	}

	FLOAT verticalLenSq = pointSphereCLenSq - projectLenSq;

	if (verticalLenSq <= sphereRadiusSq)
	{
		FLOAT inSphereLenSq = sphereRadiusSq - verticalLenSq;
		FLOAT inSphereLen = CustomType::CMath::Sqrt(inSphereLenSq);

		BOOL result = FALSE;
		FLOAT t = projectLen - inSphereLen;
		if (t <= rayDirLen)
		{
			result = TRUE;
			intersectPoints.push_back(t);
		}
		t = projectLen + inSphereLen;
		if (t <= rayDirLen)
		{
			result = TRUE;
			intersectPoints.push_back(t);
		}

		return result;
	}
	return FALSE;
}
BOOL CGameBoundSphereComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen, std::vector<FLOAT>& intersectPoints)const
{
	if (intersectPoints.size() > 0)
	{
		intersectPoints.clear();
	}

	CustomType::Vector3 sphereLocalAnchor(this->GetLocalAnchor());
	FLOAT sphereRadius = this->GetRadius();
	FLOAT sphereRadiusSq = sphereRadius * sphereRadius;

	FLOAT rayDirLenSq = rayLen * rayLen;

	CustomType::Vector3 pointSphereC(sphereLocalAnchor - rayStart);
	FLOAT pointSphereCLenSq = pointSphereC.LengthSquare();

	if (pointSphereCLenSq <= sphereRadiusSq)
	{
		intersectPoints.push_back(0.f);

		if (rayLen < 1e-3f)
		{
			return TRUE;
		}

		FLOAT projectLen = CustomType::Vector3::Dot(pointSphereC, rayDir);
		FLOAT projectLenSq = projectLen * projectLen;
		FLOAT verticalLenSq = pointSphereCLenSq - projectLenSq;

		intersectPoints.push_back(CustomType::CMath::Sqrt(sphereRadiusSq - verticalLenSq) + projectLen);

		return TRUE;
	}

	if (rayLen < 1e-3f)
	{
		return FALSE;
	}

	FLOAT projectLen = CustomType::Vector3::Dot(pointSphereC, rayDir);

	if (projectLen <= 0.f)
	{
		return FALSE;
	}

	FLOAT projectLenSq = projectLen * projectLen;

	if (projectLenSq > rayDirLenSq)
	{
		return FALSE;
	}

	FLOAT verticalLenSq = pointSphereCLenSq - projectLenSq;

	if (verticalLenSq <= sphereRadiusSq)
	{
		FLOAT inSphereLenSq = sphereRadiusSq - verticalLenSq;
		FLOAT inSphereLen = CustomType::CMath::Sqrt(inSphereLenSq);

		BOOL result = FALSE;
		FLOAT t = projectLen - inSphereLen;
		if (t <= rayLen)
		{
			result = TRUE;
			intersectPoints.push_back(t);
		}
		t = projectLen + inSphereLen;
		if (t <= rayLen)
		{
			result = TRUE;
			intersectPoints.push_back(t);
		}

		return result;
	}
	return FALSE;
}
void CGameBoundSphereComponent::Init()
{

}
void CGameBoundSphereComponent::Uninit()
{

}
#ifdef _DEVELOPMENT_EDITOR
void CGameBoundSphereComponent::SelectedEditorUpdate()
{

}
#endif



CGameBoundBoxComponent::CGameBoundBoxComponent()
{
	this->m_Anchor		= CustomType::Vector3::Zero();
	this->m_Extent[0]	= CustomStruct::CGameBaseSetting::GameBoundHalfMinimum;
	this->m_Extent[1]	= CustomStruct::CGameBaseSetting::GameBoundHalfMinimum;
	this->m_Extent[2]	= CustomStruct::CGameBaseSetting::GameBoundHalfMinimum;
}
CGameBoundBoxComponent::CGameBoundBoxComponent(const CustomType::Vector3& anchor, const CustomType::Vector3& extent)
{
	this->m_Anchor		= anchor;
	this->m_Extent[0]	= extent.X();
	this->m_Extent[1]	= extent.Y();
	this->m_Extent[2]	= extent.Z();
}
CGameBoundBoxComponent::CGameBoundBoxComponent(const CustomType::Vector3& anchor, const FLOAT& x, const FLOAT& y, const FLOAT& z)
{
	this->m_Anchor		= anchor;
	this->m_Extent[0]	= x;
	this->m_Extent[1]	= y;
	this->m_Extent[2]	= z;
}
CGameBoundBoxComponent::~CGameBoundBoxComponent()
{

}
CustomType::Vector3 CGameBoundBoxComponent::GetWorldAnchor()const
{
	if (this->m_GameObject != NULL)
	{
		CustomType::Vector3 result(this->m_GameObject->GetWorldPosition() + this->m_Anchor);
		return result;
	}
	return (this->m_Anchor);
}
const CustomType::Vector3& CGameBoundBoxComponent::GetLocalAnchor()const
{
	return (this->m_Anchor);
}
CustomType::Vector3 CGameBoundBoxComponent::GetExtent()const
{
	return (CustomType::Vector3(this->m_Extent[0], this->m_Extent[1], this->m_Extent[2]));
}
void CGameBoundBoxComponent::GetExtent(FLOAT& x, FLOAT& y, FLOAT& z)const
{
	x = this->m_Extent[0]; y = this->m_Extent[1]; z = this->m_Extent[2];
}
void CGameBoundBoxComponent::SetLocalAnchorExtent(const CustomType::Vector3& p, const CustomType::Vector3& e)
{
	this->m_Anchor		= p;
	this->m_Extent[0]	= e.X();
	this->m_Extent[1]	= e.Y();
	this->m_Extent[2]	= e.Z();
}
void CGameBoundBoxComponent::SetLocalAnchorExtent(const CustomType::Vector3& p, const FLOAT& x, const FLOAT& y, const FLOAT& z)
{
	this->m_Anchor		= p;
	this->m_Extent[0]	= x;
	this->m_Extent[1]	= y;
	this->m_Extent[2]	= z;
}
void CGameBoundBoxComponent::SetLocalAnchor(const CustomType::Vector3& v)
{
	this->m_Anchor = v;
}
void CGameBoundBoxComponent::SetExtent(const CustomType::Vector3& v)
{
	this->m_Extent[0] = v.X();
	this->m_Extent[1] = v.Y();
	this->m_Extent[2] = v.Z();
}
void CGameBoundBoxComponent::SetExtent(const FLOAT& x, const FLOAT& y, const FLOAT& z)
{
	this->m_Extent[0] = x;
	this->m_Extent[1] = y;
	this->m_Extent[2] = z;
}
BOOL CGameBoundBoxComponent::InsertCameraFrustum(const CustomStruct::CCullingFrustumInfo& cullingFrustum)const
{
	CustomType::Vector3 boxLocalAnchor(this->GetLocalAnchor());
	CustomType::Vector3 boxExtent(this->GetExtent());

	const CGameObject* gameObject = this->GetGameObject();
	CustomType::Quaternion boxR(CustomType::Quaternion::Identity());
	CustomType::Vector3 boxW(CustomType::Vector3::Zero());
	if (gameObject != NULL)
	{
		boxW = gameObject->GetWorldPosition();
		boxR = gameObject->GetWorldRotation();
	}

	// Use diagonal line to early exit this function.
	auto checkPointInsertCameraFrustum = [&](const UINT& index)->BOOL {
		CustomType::Vector3 tempPoint(boxLocalAnchor + boxExtent * _GStaticBoxVector[index]);
		if (gameObject != NULL)
		{
			tempPoint = boxR.MultiplyVector(tempPoint) + boxW;
		}
		FLOAT dotTerm = CustomType::Vector3::Dot(tempPoint, cullingFrustum.CameraForwardVec);
		if (dotTerm < cullingFrustum.CameraProjectNear || -dotTerm < cullingFrustum.CameraProjectFar)
		{
			return FALSE;
		}
		for (UINT i = 0u; i < 4u; i++)
		{
			dotTerm = CustomType::Vector3::Dot(tempPoint, cullingFrustum.CameraFrustumPlane[i]);
			if (dotTerm < cullingFrustum.CameraProjectPlane[i])
			{
				return FALSE;
			}
		}
		return TRUE;
	};

	for (UINT i = 0u; i < 8u; i++)
	{
		if (checkPointInsertCameraFrustum(i))
		{
			return TRUE;
		}
	}

	return FALSE;
}
CustomStruct::CRect CGameBoundBoxComponent::GetScreenCoordRect(const CCamera* camera)const
{
	CustomType::Vector3 boxLocalAnchor(this->GetLocalAnchor());
	CustomType::Vector3 boxExtent(this->GetExtent());

	const CGameObject* gameObject = this->GetGameObject();
	CustomType::Quaternion boxR(CustomType::Quaternion::Identity());
	CustomType::Vector3 boxW(CustomType::Vector3::Zero());
	if (gameObject != NULL)
	{
		boxW = gameObject->GetWorldPosition();
		boxR = gameObject->GetWorldRotation();
	}

	FLOAT screenMin[2] = { ENGINE_FLOAT32_MAX, ENGINE_FLOAT32_MAX }, screenMax[2] = { 0.f, 0.f };
	auto checkMinMax = [&screenMin, &screenMax](const CustomType::Vector2& input) {
		screenMin[0] = CustomType::CMath::Min(input.X(), screenMin[0]);
		screenMin[1] = CustomType::CMath::Min(input.Y(), screenMin[1]);
		screenMax[0] = CustomType::CMath::Max(input.X(), screenMax[0]);
		screenMax[1] = CustomType::CMath::Max(input.Y(), screenMax[1]); };
	auto checkScreenCoordMinMax = [&](const UINT& index, CustomType::Vector2& output) {
		CustomType::Vector3 tempPoint(boxLocalAnchor + boxExtent * _GStaticBoxVector[index]);
		if (gameObject != NULL)
		{
			tempPoint = boxR.MultiplyVector(tempPoint) + boxW;
		}
		camera->TransformWorldPointToScreenCoord(tempPoint, output); };
	{
		CustomType::Vector2 tempBoxBoundPoint;
		for (UINT i = 0u; i < 8u; i++)
		{
			checkScreenCoordMinMax(i, tempBoxBoundPoint);
			checkMinMax(tempBoxBoundPoint);
		}
	}

	return (CustomStruct::CRect(screenMin[0], screenMin[1], screenMax[0], screenMax[1]));
}
#ifdef _DEVELOPMENT_EDITOR
BOOL CGameBoundBoxComponent::SelectedInEditorScreen(const CustomType::Vector2& mousePos, const CCamera* camera, const CustomStruct::CCullingFrustumInfo& cullingFrustum, CustomType::Vector2& screenAnchor, FLOAT& viewDepth)const
{
	CustomType::Vector3 boxLocalAnchor(this->GetLocalAnchor());
	CustomType::Vector3 boxExtent(this->GetExtent());

	const CGameObject* gameObject = this->GetGameObject();
	CustomType::Quaternion boxR(CustomType::Quaternion::Identity());
	CustomType::Vector3 boxW(CustomType::Vector3::Zero());
	if (gameObject != NULL)
	{
		boxW = gameObject->GetWorldPosition();
		boxR = gameObject->GetWorldRotation();
	}

	CustomType::Vector3 newPoints[8];

	// Use diagonal line to early exit this function.
	auto checkPointInsertCameraFrustum = [&](const UINT& index)->BOOL {
		CustomType::Vector3 tempPoint(boxLocalAnchor + boxExtent * _GStaticBoxVector[index]);
		if (gameObject != NULL)
		{
			tempPoint = boxR.MultiplyVector(tempPoint) + boxW;
		}
		newPoints[index] = tempPoint;
		FLOAT dotTerm = CustomType::Vector3::Dot(tempPoint, cullingFrustum.CameraForwardVec);
		if (dotTerm < cullingFrustum.CameraProjectNear || -dotTerm < cullingFrustum.CameraProjectFar)
		{
			return FALSE;
		}
		for (UINT i = 0u; i < 4u; i++)
		{
			dotTerm = CustomType::Vector3::Dot(tempPoint, cullingFrustum.CameraFrustumPlane[i]);
			if (dotTerm < cullingFrustum.CameraProjectPlane[i])
			{
				return FALSE;
			}
		}
		return TRUE; };
	auto checkBoxInsertCameraFrustum = [&]()->BOOL {
		for (UINT i = 0u; i < 8u; i++)
		{
			if (checkPointInsertCameraFrustum(i))
			{
				return TRUE;
			}
		}
		return FALSE; };

	if (checkBoxInsertCameraFrustum())
	{
		FLOAT screenMin[2] = { ENGINE_FLOAT32_MAX, ENGINE_FLOAT32_MAX }, screenMax[2] = { 0.f, 0.f };

		{
			auto checkMinMax = [&screenMin, &screenMax](const CustomType::Vector2& input) {
				screenMin[0] = CustomType::CMath::Min(input.X(), screenMin[0]);
				screenMin[1] = CustomType::CMath::Min(input.Y(), screenMin[1]);
				screenMax[0] = CustomType::CMath::Max(input.X(), screenMax[0]);
				screenMax[1] = CustomType::CMath::Max(input.Y(), screenMax[1]); };
			CustomType::Vector2 tempScreenCoord;
			auto boxMinMaxInScreenCoord = [&](const UINT& index) {
				camera->TransformWorldPointToScreenCoord(newPoints[index], tempScreenCoord);
				checkMinMax(tempScreenCoord);
			};
			for (UINT i = 0u; i < 8u; i++)
			{
				boxMinMaxInScreenCoord(i);
			}
		}

		{
			auto checkMouseInBox = [&screenMin, &screenMax](const CustomType::Vector2& input)->BOOL {
				if (input.X() > screenMin[0] && input.X() < screenMax[0] &&
					input.Y() > screenMin[1] && input.Y() < screenMax[1])
				{
					return TRUE;
				}
				return FALSE;
			};
			if (checkMouseInBox(mousePos))
			{
				CustomType::Vector3 boxAnchorWS(this->GetWorldAnchor());
				CustomType::Vector3 boxAnchorVS;
				camera->TransformPointFromWorldToView(boxAnchorWS, boxAnchorVS);
				viewDepth = boxAnchorVS.Z();
				camera->TransformWorldPointToScreenCoord(boxAnchorWS, screenAnchor);
				return TRUE;
			}
		}
	}

	return FALSE;
}
#endif
BOOL CGameBoundBoxComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd)const
{
	//TODO
	return FALSE;
}
BOOL CGameBoundBoxComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen)const
{
	//TODO
	return FALSE;
}
BOOL CGameBoundBoxComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd, std::vector<FLOAT>& intersectPoints)const
{
	//TODO
	return FALSE;
}
BOOL CGameBoundBoxComponent::RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen, std::vector<FLOAT>& intersectPoints)const
{
	//TODO
	return FALSE;
}
void CGameBoundBoxComponent::Init()
{

}
void CGameBoundBoxComponent::Uninit()
{

}
#ifdef _DEVELOPMENT_EDITOR
void CGameBoundBoxComponent::SelectedEditorUpdate()
{

}
#endif