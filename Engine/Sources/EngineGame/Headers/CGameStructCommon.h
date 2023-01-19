#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"

namespace CustomStruct
{
	class CGameBaseSetting
	{
	public:
		constexpr static FLOAT GameBoundMinimum			= 0.1f;
		constexpr static FLOAT GameBoundHalfMinimum		= 0.05f;
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
		CCullingFrustumInfo()
		{
			this->CameraProjectPlane[0] = 0.f;
			this->CameraProjectPlane[1] = 0.f;
			this->CameraProjectPlane[2] = 0.f;
			this->CameraProjectPlane[3] = 0.f;
			this->CameraProjectNear		= 0.f;
			this->CameraProjectFar		= 0.f;
		}
		CCullingFrustumInfo(const CCullingFrustumInfo& v)
		{
			CameraForwardVec		= v.CameraForwardVec;
			CameraProjectPlane[0]	= v.CameraProjectPlane[0];
			CameraProjectPlane[1]	= v.CameraProjectPlane[1];
			CameraProjectPlane[2]	= v.CameraProjectPlane[2];
			CameraProjectPlane[3]	= v.CameraProjectPlane[3];
			CameraProjectNear		= v.CameraProjectNear;
			CameraProjectFar		= v.CameraProjectFar;
			CameraFrustumPlane[0]	= v.CameraFrustumPlane[0];
			CameraFrustumPlane[1]	= v.CameraFrustumPlane[1];
			CameraFrustumPlane[2]	= v.CameraFrustumPlane[2];
			CameraFrustumPlane[3]	= v.CameraFrustumPlane[3];
		}
		CustomType::Vector3		CameraForwardVec;
		FLOAT					CameraProjectPlane[4];
		FLOAT					CameraProjectNear;
		FLOAT					CameraProjectFar;
		CustomType::Vector3		CameraFrustumPlane[4];
	};

	struct CGameBoneNodeInfo
	{
		CGameBoneNodeInfo()
		{
			this->Index		= -1;
			this->Location	= CustomType::Vector3::Zero();
			this->Rotation	= CustomType::Quaternion::Identity();
			this->Scale		= CustomType::Vector3::One();
			this->Offset	= CustomType::Matrix4x4::Identity();
			this->Parent	= nullptr;
		}
		CGameBoneNodeInfo(const std::string& name)
		{
			this->Index		= -1;
			this->Name		= name;
			this->Location	= CustomType::Vector3::Zero();
			this->Rotation	= CustomType::Quaternion::Identity();
			this->Scale		= CustomType::Vector3::One();
			this->Offset	= CustomType::Matrix4x4::Identity();
			this->Parent	= nullptr;
		}
		INT									Index;
		std::string							Name;
		CustomType::Vector3					Location;
		CustomType::Quaternion				Rotation;
		CustomType::Vector3					Scale;
		CustomType::Matrix4x4				Offset;
		CGameBoneNodeInfo*					Parent;
		std::vector<CGameBoneNodeInfo*>		Children;
	};
}