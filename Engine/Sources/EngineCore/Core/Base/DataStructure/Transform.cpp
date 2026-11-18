#include "Transform.h"

namespace PigeonEngine
{

	ETransform::ETransform()
		: LocalLocation(Vector3::Zero())
		, LocalRotation(Quaternion::Identity())
		, LocalScaling(Vector3::One())
	{
	}
	ETransform::ETransform(const Vector3& InLocation, const Quaternion& InRotation, const Vector3& InScaling)
		: LocalLocation(InLocation), LocalRotation(InRotation), LocalScaling(InScaling)
	{
	}
	ETransform::ETransform(const ETransform& other)
		: LocalLocation(other.LocalLocation), LocalRotation(other.LocalRotation), LocalScaling(other.LocalScaling)
	{
	}
	ETransform& ETransform::operator=(const ETransform& other)
	{
		LocalLocation = other.LocalLocation;
		LocalRotation = other.LocalRotation;
		LocalScaling = other.LocalScaling;
		return (*this);
	}
	ETransform::~ETransform()
	{
	}

	Matrix4x4 ETransform::ToMatrix() const
	{
		// Row-vector convention (DirectXMath): scale, then rotate, then translate.
		return MakeMatrix4x4(LocalLocation, LocalRotation, LocalScaling);
	}

	ETransform ETransform::FromMatrix(const Matrix4x4& InMatrix)
	{
		// Row-vector convention: basis vectors are rows 0-2, translation is row 3.
		// XMMatrixDecompose takes the scale out first, which is what lets the rotation be
		// recovered from a matrix that may still carry a parent's non-uniform scaling.
		DirectX::XMVECTOR Scale;
		DirectX::XMVECTOR Rotation;
		DirectX::XMVECTOR Translation;
		if (!DirectX::XMMatrixDecompose(&Scale, &Rotation, &Translation, InMatrix.GetDirectXMatrix()))
		{
			// A degenerate matrix (zero scale on an axis) cannot be decomposed; keep the
			// translation and fall back to identity rotation and unit scale rather than
			// handing NaNs to callers.
			return ETransform(Vector3(InMatrix._41, InMatrix._42, InMatrix._43), Quaternion::Identity(), Vector3::One());
		}
		return ETransform(Vector3(Translation), Quaternion(Rotation), Vector3(Scale));
	}

	Vector3 ETransform::GetLocation_Local() const
	{
		return LocalLocation;
	}
	Quaternion ETransform::GetRotation_Local() const
	{
		return LocalRotation;
	}
	Vector3 ETransform::GetScaling_Local() const
	{
		return LocalScaling;
	}
	Vector3 ETransform::GetForwardVector_Local() const
	{
		return SplitForwardVector(LocalRotation);
	}
	Vector3 ETransform::GetUpVector_Local() const
	{
		return SplitUpVector(LocalRotation);
	}
	Vector3 ETransform::GetRightVector_Local() const
	{
		return SplitRightVector(LocalRotation);
	}

	void ETransform::SetLocation_Local(const Vector3& NewValue)
	{
		LocalLocation = NewValue;
	}
	void ETransform::SetRotation_Local(const Quaternion& NewValue)
	{
		LocalRotation = NewValue;
	}
	void ETransform::SetScaling_Local(const Vector3& NewValue)
	{
		LocalScaling = NewValue;
	}

};
