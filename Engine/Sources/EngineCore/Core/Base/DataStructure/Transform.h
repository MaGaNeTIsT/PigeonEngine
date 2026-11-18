#pragma once

#include "../../../Main/Main.h"
#include "BuiltInType.h"
#include "BaseType.h"
#include <Config/ErrorCaption.h>
#include "../../../../Development/Alert/DevelopmentDefines.h"

namespace PigeonEngine
{

	enum class EMobilityType : UINT8
	{
		EMT_STATIC,
		EMT_DYNAMIC,
		EMT_STATION
	};

	// How an attach treats the child's transform. The legacy 2-argument attach overloads
	// map to KEEP_CURRENT, so every existing call site keeps its behavior.
	enum class EAttachmentRuleType : UINT8
	{
		KEEP_CURRENT,	// keep the child's current local transform (historic default)
		KEEP_WORLD,		// keep the child's world transform (runtime attachments)
		KEEP_RELATIVE,	// apply the provided relative transform
		SNAP_TO_TARGET,	// local transform becomes identity
	};

	/*
	* A transform is a plain TRS value: local location, rotation and scaling. It does not
	* walk the parent chain - PSceneComponent owns the world-space cache and composes
	* matrices along the component chain (World = Local * ParentWorld under the engine's
	* row-vector convention), so this type stays copyable, allocation free and cheap to
	* pass by value.
	*/
	class ETransform
	{
	public:
		ETransform();
		ETransform(const Vector3& InLocation, const Quaternion& InRotation, const Vector3& InScaling);
		ETransform(const ETransform& Other);
		ETransform& operator=(const ETransform& Other);
		~ETransform();
	public:
		// Row-vector convention (DirectXMath): ToMatrix builds Scale * Rotation * Translation.
		PE_NODISCARD Matrix4x4	ToMatrix() const;
		// Inverse of ToMatrix: translation comes back exactly, rotation and scale through a
		// matrix decomposition (which folds a negative scale onto one axis).
		static ETransform		FromMatrix(const Matrix4x4& InMatrix);
	public:
		PE_NODISCARD Vector3	GetLocation_Local() const;
		PE_NODISCARD Quaternion	GetRotation_Local() const;
		PE_NODISCARD Vector3	GetScaling_Local() const;
		PE_NODISCARD Vector3	GetForwardVector_Local() const;
		PE_NODISCARD Vector3	GetUpVector_Local() const;
		PE_NODISCARD Vector3	GetRightVector_Local() const;
	public:
		void	SetLocation_Local(const Vector3& NewValue);
		void	SetRotation_Local(const Quaternion& NewValue);
		void	SetScaling_Local(const Vector3& NewValue);
	protected:
		Vector3		LocalLocation;
		Quaternion	LocalRotation;
		Vector3		LocalScaling;

	};

};
