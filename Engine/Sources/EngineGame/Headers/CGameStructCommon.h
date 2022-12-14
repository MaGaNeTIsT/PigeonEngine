#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"

namespace CustomStruct
{
	class CGameBaseSetting
	{
	public:
		constexpr static FLOAT GameBoundMinimum			= 5.f;
		constexpr static FLOAT GameBoundHalfMinimum		= 2.5f;
	};

	struct CRect
	{
		CRect() { ::ZeroMemory(this, sizeof(*this)); }
		CRect(const FLOAT& left, const FLOAT& top, const FLOAT& right, const FLOAT& bottom) { Left = left; Top = top; Right = right; Bottom = bottom; }
		FLOAT	Left;
		FLOAT	Top;
		FLOAT	Right;
		FLOAT	Bottom;
	};

	struct CCullingFrustumInfo
	{
		CCullingFrustumInfo() { ZeroMemory(this, sizeof(*this)); }
		CCullingFrustumInfo(const CCullingFrustumInfo& v)
		{
			this->CameraForwardVec		= v.CameraForwardVec;
			this->CameraProjectPlane[0] = v.CameraProjectPlane[0];
			this->CameraProjectPlane[1] = v.CameraProjectPlane[1];
			this->CameraProjectPlane[2] = v.CameraProjectPlane[2];
			this->CameraProjectPlane[3] = v.CameraProjectPlane[3];
			this->CameraProjectNear		= v.CameraProjectNear;
			this->CameraProjectFar		= v.CameraProjectFar;
			this->CameraFrustumPlane[0] = v.CameraFrustumPlane[0];
			this->CameraFrustumPlane[1] = v.CameraFrustumPlane[1];
			this->CameraFrustumPlane[2] = v.CameraFrustumPlane[2];
			this->CameraFrustumPlane[3] = v.CameraFrustumPlane[3];
		}
		CustomType::Vector3		CameraForwardVec;
		FLOAT					CameraProjectPlane[4];
		FLOAT					CameraProjectNear;
		FLOAT					CameraProjectFar;
		CustomType::Vector3		CameraFrustumPlane[4];
	};
}