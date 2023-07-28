/*
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
			this->Index				= -2;
			this->Name				= "_BoneDefaultNullName";
			this->DefaultPosition	= CustomType::Vector3::Zero();
			this->DefaultRotation	= CustomType::Quaternion::Identity();
			this->DefaultScaling	= CustomType::Vector3::One();
			this->BindPoseMatrix	= CustomType::Matrix4x4::Identity();
			this->Parent			= -2;
			if (this->Children.size() > 0)
			{
				this->Children.clear();
			}
		}
		CGameBoneNodeInfo(const std::string& name)
		{
			this->Index				= -2;
			this->Name				= name;
			this->DefaultPosition	= CustomType::Vector3::Zero();
			this->DefaultRotation	= CustomType::Quaternion::Identity();
			this->DefaultScaling	= CustomType::Vector3::One();
			this->BindPoseMatrix	= CustomType::Matrix4x4::Identity();
			this->Parent			= -2;
			if (this->Children.size() > 0)
			{
				this->Children.clear();
			}
		}
		SHORT								Index;
		std::string							Name;
		CustomType::Vector3					DefaultPosition;
		CustomType::Quaternion				DefaultRotation;
		CustomType::Vector3					DefaultScaling;
		CustomType::Matrix4x4				BindPoseMatrix;
		SHORT								Parent;
		std::vector<SHORT>					Children;
	};

	template<typename ValueType>
	struct CGameAnimationKey
	{
		CGameAnimationKey()
		{
			this->Time = static_cast<DOUBLE>(0);
		}
		BOOL operator>(const CGameAnimationKey<ValueType>& k)
		{
			return (this->Time > k.Time);
		}
		BOOL operator<(const CGameAnimationKey<ValueType>& k)
		{
			return (this->Time < k.Time);
		}
		BOOL operator>=(const CGameAnimationKey<ValueType>& k)
		{
			return (this->Time >= k.Time);
		}
		BOOL operator<=(const CGameAnimationKey<ValueType>& k)
		{
			return (this->Time <= k.Time);
		}
		DOUBLE		Time;
		ValueType	Value;
	};

	enum CGameAnimationBehaviour
	{
		ANIMATION_BEHAVIOUR_DEFAULT		= 0,
		ANIMATION_BEHAVIOUR_CONSTANT	= 1,
		ANIMATION_BEHAVIOUR_LINEAR		= 2,
		ANIMATION_BEHAVIOUR_REPEAT		= 3
	};

	struct CGameAnimationNodeInfo
	{
		CGameAnimationNodeInfo()
		{
			this->PreState	= CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_DEFAULT;
			this->PostState	= CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_DEFAULT;
		}
		CGameAnimationNodeInfo(const std::string& name)
		{
			this->Name		= name;
			this->PreState	= CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_DEFAULT;
			this->PostState	= CGameAnimationBehaviour::ANIMATION_BEHAVIOUR_DEFAULT;
		}
		std::string					Name;
		CGameAnimationBehaviour		PreState;
		CGameAnimationBehaviour		PostState;
		std::vector<CGameAnimationKey<CustomType::Vector3>>		PositionKeys;
		std::vector<CGameAnimationKey<CustomType::Quaternion>>	RotationKeys;
		std::vector<CGameAnimationKey<CustomType::Vector3>>		ScalingKeys;
	};

	struct CGameAnimationInfo
	{
		CGameAnimationInfo()
		{
			this->Duration			= static_cast<DOUBLE>(0);
			this->TicksPerSecond	= static_cast<DOUBLE>(0);
		}
		CGameAnimationInfo(const std::string& name)
		{
			this->Name				= name;
			this->Duration			= static_cast<DOUBLE>(0);
			this->TicksPerSecond	= static_cast<DOUBLE>(0);
		}
		std::string		Name;
		DOUBLE			Duration;
		DOUBLE			TicksPerSecond;
		std::vector<CGameAnimationNodeInfo>		AnimationNodes;
	};
}
*/