#include "ShaderParameter.h"

namespace PigeonEngine
{

	PE_INLINE DirectX::XMFLOAT4X4 TranslateUploadType(const Matrix4x4& InData)
	{
		DirectX::XMFLOAT4X4 Result(InData.GetDirectXTransposeValue4x4());
		return Result;
	}
	PE_INLINE DirectX::XMFLOAT4 TranslateUploadType(const Vector4& InData)
	{
		DirectX::XMFLOAT4 Result(InData.GetDirectXValue4());
		return Result;
	}

	EMaterialParameter::EMaterialParameter()
	{
		SetupParameters();
	}
	EMaterialParameter::EMaterialParameter(const EMaterialParameter& Other)
	{
		SetupParameters();
	}
	EMaterialParameter::~EMaterialParameter()
	{
		ReleaseParameter();
	}
	void EMaterialParameter::ReleaseParameter()
	{
		InitCommands.DoCommands();
		InitCommands.EmptyQueue();
		RawData.Release();
		ShaderParameterNames.Clear();
		ShaderParameters.Clear();
	}

};