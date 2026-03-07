#include "RenderSingletonObject.h"
#include <RenderResource.h>
#include <RenderDevice/DeviceD3D11.h>
#if _EDITOR_ONLY
#include <RenderConfig/RenderConfig.h>
#include <Shader/ShaderParameter.h>
#include <ShaderAsset/ShaderAsset.h>
#include "../../Editor/EditorLogManager.h"
#include <RenderProxy/ViewProxy.h>
#endif

namespace PigeonEngine
{
	static RBufferResource _GFullScreenTriangleVertexBuffer;
	static RBufferResource _GFullScreenTriangleIndexBuffer;

	static void CreateFullScreenTriangleVertexBuffer()
	{
		if (!(_GFullScreenTriangleVertexBuffer.IsRenderResourceValid()))
		{
			FLOAT FullScreenTriangleVertex[] =
			{
				0.f, 1.f,
				1.f, 1.f,
				0.f, 0.f,
				1.f, 0.f
			};
			RSubresourceDataDesc SubresourceDataDesc;
			SubresourceDataDesc.pSysMem = FullScreenTriangleVertex;
#if _EDITOR_ONLY
			BOOL32 Result =
#endif
				RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(_GFullScreenTriangleVertexBuffer.Buffer,
					RBufferDesc((sizeof(FLOAT) * 2u * 4u), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
					(&SubresourceDataDesc));
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create full screen triangle vertex buffer failed."));
			}
#endif
		}
	}
	static void CreateFullScreenTriangleIndexBuffer()
	{
		if (!(_GFullScreenTriangleIndexBuffer.IsRenderResourceValid()))
		{
			USHORT FullScreenTriangleIndex[] =
			{
				0, 1, 2,
				2, 1, 3
			};
			RSubresourceDataDesc SubresourceDataDesc;
			SubresourceDataDesc.pSysMem = FullScreenTriangleIndex;
#if _EDITOR_ONLY
			BOOL32 Result =
#endif
				RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(_GFullScreenTriangleIndexBuffer.Buffer,
					RBufferDesc((2u * 6u), RBindFlagType::BIND_INDEX_BUFFER, 2u),
					(&SubresourceDataDesc));
#if _EDITOR_ONLY
			if (!Result)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create full screen triangle index buffer failed."));
			}
#endif
		}
	}

	RFullScreenTriangle::RFullScreenTriangle()
	{
		CreateFullScreenTriangleVertexBuffer();
		CreateFullScreenTriangleIndexBuffer();
	}
	RFullScreenTriangle::RFullScreenTriangle(const RFullScreenTriangle& Other)
	{
		CreateFullScreenTriangleVertexBuffer();
		CreateFullScreenTriangleIndexBuffer();
	}
	RFullScreenTriangle::~RFullScreenTriangle()
	{

	}
	RFullScreenTriangle& RFullScreenTriangle::operator=(const RFullScreenTriangle& Other)
	{
		CreateFullScreenTriangleVertexBuffer();
		CreateFullScreenTriangleIndexBuffer();
		return (*this);
	}
	UINT32 RFullScreenTriangle::GetIndexCount()const
	{
		return 6u;	//TODO
	}
	void RFullScreenTriangle::BindPrimitiveBuffers()
	{
#if _DEBUG_MODE
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check full screen triangle vertex buffer is invalid."), (_GFullScreenTriangleVertexBuffer.IsRenderResourceValid()));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check full screen triangle index buffer is invalid."), (_GFullScreenTriangleIndexBuffer.IsRenderResourceValid()));
#endif
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RenderDevice->SetIndexBuffer(_GFullScreenTriangleIndexBuffer.Buffer, 0u, RFormatType::FORMAT_R16_UINT);
		RenderDevice->SetVertexBuffer(_GFullScreenTriangleVertexBuffer.Buffer, (sizeof(FLOAT) * 2u), 0u, 0u);
	}

#if _EDITOR_ONLY
	const EVertexShaderAsset* _GDebugWireframePrimitiveVS = nullptr;
	const EPixelShaderAsset* _GDebugWireframePrimitivePS = nullptr;

	RDebugWireframePrimitive::RDebugWireframePrimitive()
		: UseShortIndex(FALSE), IndexCount(0u)
	{
		Bound.IsValid = TRUE;
	}
	RDebugWireframePrimitive::RDebugWireframePrimitive(const RDebugWireframePrimitive& Other)
		: UseShortIndex(Other.UseShortIndex), IndexCount(Other.IndexCount), Bound(Other.Bound), VertexBuffer(Other.VertexBuffer), IndexBuffer(Other.IndexBuffer)
	{
		Bound.IsValid = TRUE;
	}
	RDebugWireframePrimitive::~RDebugWireframePrimitive()
	{
	}
	RDebugWireframePrimitive& RDebugWireframePrimitive::operator=(const RDebugWireframePrimitive& Other)
	{
		UseShortIndex	= Other.UseShortIndex;
		IndexCount		= Other.IndexCount;
		Bound			= Other.Bound;
		VertexBuffer	= Other.VertexBuffer;
		IndexBuffer		= Other.IndexBuffer;
		return (*this);
	}
	BOOL32 RDebugWireframePrimitive::InitPrimitive(const Vector3* InVertexDatas, const UINT32 InVertexNum)
	{
		if (InVertexNum == 0u)
		{
			return FALSE;
		}

		const UINT32 LineCount = InVertexNum - 1u;

		if (VertexBuffer.IsRenderResourceValid() && IndexBuffer.IsRenderResourceValid())
		{
			if (IndexCount == (LineCount * 2u))
			{
				return TRUE;
			}
			VertexBuffer.ReleaseRenderResource();
			IndexBuffer.ReleaseRenderResource();
		}

		UseShortIndex = (InVertexNum <= 65536u) ? TRUE : FALSE;
		IndexCount = LineCount * 2u;

		UINT32* UintIndices = nullptr;
		USHORT* UshortIndices = nullptr;
		if (UseShortIndex)
		{
			UshortIndices = new USHORT[IndexCount];
			for (USHORT i = 0u, n = static_cast<USHORT>(LineCount); i < n; i++)
			{
				if ((static_cast<UINT32>(i * 2u + 1u)) < IndexCount)
				{
					UshortIndices[i * 2u + 0u] = i;
					UshortIndices[i * 2u + 1u] = i + 1u;
				}
			}
		}
		else
		{
			UintIndices = new UINT32[IndexCount];
			for (UINT32 i = 0u; i < LineCount; i++)
			{
				if ((i * 2u + 1u) < IndexCount)
				{
					UintIndices[i * 2u + 0u] = i;
					UintIndices[i * 2u + 1u] = i + 1u;
				}
			}
		}

		RSubresourceDataDesc SubresourceDataDesc;
		SubresourceDataDesc.pSysMem = InVertexDatas;

		BOOL32 Result = RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(VertexBuffer.Buffer,
			RBufferDesc((sizeof(Vector3) * InVertexNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
			(&SubresourceDataDesc));

		SubresourceDataDesc.pSysMem = UseShortIndex ? ((const void*)UshortIndices) : ((const void*)UintIndices);

		Result = Result && RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(IndexBuffer.Buffer,
			RBufferDesc(((UseShortIndex ? 2u : 4u) * IndexCount), RBindFlagType::BIND_INDEX_BUFFER, (UseShortIndex ? 2u : 4u)),
			(&SubresourceDataDesc));

		if (!!UintIndices)
		{
			delete[]UintIndices;
		}
		if (!!UshortIndices)
		{
			delete[]UshortIndices;
		}

		{
			Bound.AABBMin = Vector3(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
			Bound.AABBMax = Vector3(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
			for (UINT32 i = 0u; i < InVertexNum; i++)
			{
				Bound.AABBMin = MinVector3(InVertexDatas[i], Bound.AABBMin);
				Bound.AABBMax = MaxVector3(InVertexDatas[i], Bound.AABBMax);
			}
		}

		return Result;
	}
	BOOL32 RDebugWireframePrimitive::InitPrimitive(const Vector3* InVertexDatas, const UINT32 InVertexNum, const UINT32* InIndexDatas, const UINT32 InIndexNum)
	{
		if ((InVertexNum == 0u) || (InIndexNum == 0u) || ((InIndexNum & 0x1u) != 0u))
		{
			return FALSE;
		}

		if (VertexBuffer.IsRenderResourceValid() && IndexBuffer.IsRenderResourceValid())
		{
			if (IndexCount == InIndexNum)
			{
				return TRUE;
			}
			VertexBuffer.ReleaseRenderResource();
			IndexBuffer.ReleaseRenderResource();
		}

		UseShortIndex = (InVertexNum <= 65536u) ? TRUE : FALSE;
		IndexCount = InIndexNum;

		USHORT* UshortIndices = nullptr;
		if (UseShortIndex)
		{
			UshortIndices = new USHORT[IndexCount];
			for (UINT32 i = 0u; i < InIndexNum; i++)
			{
				UshortIndices[i] = static_cast<USHORT>(InIndexDatas[i]);
			}
		}

		RSubresourceDataDesc SubresourceDataDesc;
		SubresourceDataDesc.pSysMem = InVertexDatas;

		BOOL32 Result = RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(VertexBuffer.Buffer,
			RBufferDesc((sizeof(Vector3) * InVertexNum), RBindFlagType::BIND_VERTEX_BUFFER, 4u),
			(&SubresourceDataDesc));

		SubresourceDataDesc.pSysMem = UseShortIndex ? ((const void*)UshortIndices) : ((const void*)InIndexDatas);

		Result = Result && RDeviceD3D11::GetDeviceSingleton()->CreateBuffer(IndexBuffer.Buffer,
			RBufferDesc(((UseShortIndex ? 2u : 4u) * IndexCount), RBindFlagType::BIND_INDEX_BUFFER, (UseShortIndex ? 2u : 4u)),
			(&SubresourceDataDesc));

		if (!!UshortIndices)
		{
			delete[]UshortIndices;
		}

		{
			Bound.AABBMin = Vector3(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
			Bound.AABBMax = Vector3(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
			for (UINT32 i = 0u; i < InVertexNum; i++)
			{
				Bound.AABBMin = MinVector3(InVertexDatas[i], Bound.AABBMin);
				Bound.AABBMax = MaxVector3(InVertexDatas[i], Bound.AABBMax);
			}
		}

		return Result;
	}
	BOOL32 RDebugWireframePrimitive::IsUseShortIndex()const
	{
		return UseShortIndex;
	}
	UINT32 RDebugWireframePrimitive::GetIndexCount()const
	{
		return IndexCount;
	}
	const EBoundAABB& RDebugWireframePrimitive::GetBound()const
	{
		return Bound;
	}
	const RBufferResource& RDebugWireframePrimitive::GetPrimitiveVertexBuffer()const
	{
		return VertexBuffer;
	}
	const RBufferResource& RDebugWireframePrimitive::GetPrimitiveIndexBuffer()const
	{
		return IndexBuffer;
	}

	RDebugWireframePrimitiveManager::RDebugWireframePrimitiveManager()
	{
	}
	RDebugWireframePrimitiveManager::~RDebugWireframePrimitiveManager()
	{
	}
	void RDebugWireframePrimitiveManager::Initialize()
	{
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SINGLELINE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_PLANE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CIRCLE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CUBOID);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SPHERE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CONE);
		CreatePrimitive(RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CYLINDER);

		{
			const EString ImportPath(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
			if (!_GDebugWireframePrimitiveVS)
			{
				const EString ImportVSName = EString("DebugWireFramePrimitive_") + EEngineSettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
				const RInputLayoutDesc TempShaderInputLayouts[] =
				{
					RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_POSITION0, sizeof(FLOAT), 3u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT)
				};
				constexpr UINT32 TempShaderInputLayoutNum = PE_ARRAYSIZE(TempShaderInputLayouts);
				TryLoadVertexShader(EString(EBaseSettings::ENGINE_SHADER_PATH), ImportVSName,
					_GDebugWireframePrimitiveVS,
					&ImportPath, &ImportVSName,
					TempShaderInputLayouts, &TempShaderInputLayoutNum);
			}
			if (!_GDebugWireframePrimitivePS)
			{
				const EString ImportPSName = EString("DebugWireFramePrimitive_") + EEngineSettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
				TryLoadPixelShader(EString(EBaseSettings::ENGINE_SHADER_PATH), ImportPSName,
					_GDebugWireframePrimitivePS,
					&ImportPath, &ImportPSName);
			}
		}
	}
	void RDebugWireframePrimitiveManager::ShutDown()
	{
	}
	void RDebugWireframePrimitiveManager::RenderPrimitives_RenderThread(const RViewProxy* InView)
	{
		{
			for (UINT32 i = 0u, n = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u; i < n; i++)
			{
				PrimitiveTransforms[i].Empty();
				PrimitiveColors[i].Empty();
			}
			for (auto It = CustomPrimitiveTransforms.Begin(); It != CustomPrimitiveTransforms.End(); It++)
			{
				It->second.Empty();
			}
			for (auto It = CustomPrimitiveColors.Begin(); It != CustomPrimitiveColors.End(); It++)
			{
				It->second.Empty();
			}
		}

		{
			RequireCreateCustomCommands.DoCommands();
			RequireCreateCustomCommands.EmptyQueue();
			RequireCommands.DoCommands();
			RequireCommands.EmptyQueue();

#if _DEBUG_MODE
			for (UINT32 i = 0u, n = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u; i < n; i++)
			{
				Check(PrimitiveTransforms[i].Num() == PrimitiveColors[i].Num());
			}
			Check(CustomPrimitiveTransforms.Num() == CustomPrimitiveColors.Num());
#endif
		}

		{
			constexpr UINT32 TransformSize = sizeof(Matrix4x4) * PrimitiveDrawableMaxNum;
			constexpr UINT32 ColorSize = sizeof(Color4) * PrimitiveDrawableMaxNum;

			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();

			RenderDevice->SetVSShader(_GDebugWireframePrimitiveVS->GetRenderResource()->Shader);
			RenderDevice->SetInputLayout(_GDebugWireframePrimitiveVS->GetRenderResource()->InputLayout);
			RenderDevice->SetPSShader(_GDebugWireframePrimitivePS->GetRenderResource()->Shader);

			for (UINT32 i = 0u, n = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_COUNT - 1u; i < n; i++)
			{
				if (!(PrimitiveTransformsBuffer[i].IsRenderResourceValid()))
				{
					RenderDevice->CreateStructuredBuffer(PrimitiveTransformsBuffer[i], RStructuredBufferDesc(sizeof(Matrix4x4), PrimitiveDrawableMaxNum));
				}
				if (!(PrimitiveColorsBuffer[i].IsRenderResourceValid()))
				{
					RenderDevice->CreateStructuredBuffer(PrimitiveColorsBuffer[i], RStructuredBufferDesc(sizeof(Color4), PrimitiveDrawableMaxNum));
				}

				const TArray<Matrix4x4>& TempTransforms = PrimitiveTransforms[i];
				const TArray<Color4>& TempColors = PrimitiveColors[i];
				Check(TempTransforms.Num() == TempColors.Num());

				if (const UINT32 InstanceNum = TempTransforms.Num<UINT32>(); InstanceNum > 0u)
				{
					Matrix4x4* TempDataTransforms = new Matrix4x4[PrimitiveDrawableMaxNum];
					Color4* TempDataColor4s = new Color4[PrimitiveDrawableMaxNum];

					for (UINT32 Index = 0u; (Index < InstanceNum) && (Index < PrimitiveDrawableMaxNum); Index++)
					{
						TempDataTransforms[Index] = TempTransforms[Index];
						TempDataColor4s[Index] = TempColors[Index];
					}

					RenderDevice->UploadBuffer(PrimitiveTransformsBuffer[i].Buffer, TempDataTransforms, TransformSize, TransformSize);
					RenderDevice->UploadBuffer(PrimitiveColorsBuffer[i].Buffer, TempDataColor4s, ColorSize, ColorSize);

					delete[]TempDataTransforms;
					delete[]TempDataColor4s;

					RenderDevice->SetVertexBuffer(DebugWireframePrimitives[i].GetPrimitiveVertexBuffer().Buffer, sizeof(FLOAT) * 3u, 0u, 0u);
					RenderDevice->SetIndexBuffer(DebugWireframePrimitives[i].GetPrimitiveIndexBuffer().Buffer, 0u, (DebugWireframePrimitives[i].IsUseShortIndex()) ? RFormatType::FORMAT_R16_UINT : RFormatType::FORMAT_R32_UINT);
					RenderDevice->BindVSShaderResourceView(PrimitiveTransformsBuffer[i].ShaderResourceView, 0u);
					RenderDevice->BindVSShaderResourceView(PrimitiveColorsBuffer[i].ShaderResourceView, 1u);
					RenderDevice->DrawIndexedInstance(InstanceNum, DebugWireframePrimitives[i].GetIndexCount());
				}
			}

			for (auto It = CustomPrimitiveTransforms.Begin(); It != CustomPrimitiveTransforms.End(); It++)
			{
				const EString& Name = (It->first);

				const TArray<Matrix4x4>& CustomTransforms = (It->second);
				TArray<Color4>* CustomColors = CustomPrimitiveColors.FindValueAsPtr(Name);
				Check(((!!CustomColors) && (CustomColors->Num() == CustomTransforms.Num())));

				if (!(CustomPrimitiveTransformsBuffer.ContainsKey(Name)))
				{
					CustomPrimitiveTransformsBuffer.Add(Name, RStructuredBuffer());
					RStructuredBuffer* Buffer = CustomPrimitiveTransformsBuffer.FindValueAsPtr(Name);
					RenderDevice->CreateStructuredBuffer((*Buffer), RStructuredBufferDesc(sizeof(Matrix4x4), PrimitiveDrawableMaxNum));
				}
				if (!(CustomPrimitiveColorsBuffer.ContainsKey(Name)))
				{
					CustomPrimitiveColorsBuffer.Add(Name, RStructuredBuffer());
					RStructuredBuffer* Buffer = CustomPrimitiveColorsBuffer.FindValueAsPtr(Name);
					RenderDevice->CreateStructuredBuffer((*Buffer), RStructuredBufferDesc(sizeof(Color4), PrimitiveDrawableMaxNum));
				}

				if (const UINT32 InstanceNum = CustomTransforms.Num<UINT32>(); InstanceNum > 0u)
				{
					RStructuredBuffer* TransformsBuffer = CustomPrimitiveTransformsBuffer.FindValueAsPtr(Name);
					RStructuredBuffer* ColorsBuffer = CustomPrimitiveColorsBuffer.FindValueAsPtr(Name);

					Matrix4x4* TempDataTransforms = new Matrix4x4[PrimitiveDrawableMaxNum];
					Color4* TempDataColor4s = new Color4[PrimitiveDrawableMaxNum];

					for (UINT32 Index = 0u; (Index < InstanceNum) && (Index < PrimitiveDrawableMaxNum); Index++)
					{
						TempDataTransforms[Index] = CustomTransforms[Index];
						TempDataColor4s[Index] = (*CustomColors)[Index];
					}

					RenderDevice->UploadBuffer(TransformsBuffer->Buffer, TempDataTransforms, TransformSize, TransformSize);
					RenderDevice->UploadBuffer(ColorsBuffer->Buffer, TempDataColor4s, ColorSize, ColorSize);

					delete[]TempDataTransforms;
					delete[]TempDataColor4s;

					RDebugWireframePrimitive* Primitive = DebugWireframeCustomPrimitives.FindValueAsPtr(Name);

					RenderDevice->SetVertexBuffer(Primitive->GetPrimitiveVertexBuffer().Buffer, sizeof(FLOAT) * 3u, 0u, 0u);
					RenderDevice->SetIndexBuffer(Primitive->GetPrimitiveIndexBuffer().Buffer, 0u, (Primitive->IsUseShortIndex()) ? RFormatType::FORMAT_R16_UINT : RFormatType::FORMAT_R32_UINT);
					RenderDevice->BindVSShaderResourceView(TransformsBuffer->ShaderResourceView, 0u);
					RenderDevice->BindVSShaderResourceView(ColorsBuffer->ShaderResourceView, 1u);
					RenderDevice->DrawIndexedInstance(InstanceNum, Primitive->GetIndexCount());
				}
			}
		}
	}
	void RDebugWireframePrimitiveManager::DrawSingleLine(const Vector3& InPos0, const Vector3& InPos1, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SINGLELINE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		Vector3 Direction = InPos1 - InPos0;
		FLOAT DirectionLength = Direction.Length();
		if (DirectionLength <= 1e-3f)
		{
			Direction = Vector3::XVector();
			DirectionLength = 1e-3f;
		}
		else
		{
			Direction = Direction / DirectionLength;
		}

		RequireCommands.EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InPos0, MakeQuaternion(Vector3::YVector(), Direction), Vector3(1.f, DirectionLength, 1.f)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawPlane(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_PLANE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands.EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InWidthX, InHeightY, 1.f)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCircle(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InRadius, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CIRCLE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands.EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InRadius, InRadius, 1.f)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCuboid(const Vector3& InLocation, const Quaternion& InRotation, const FLOAT InWidthX, const FLOAT InHeightY, const FLOAT InLengthZ, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CUBOID - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands.EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InWidthX, InHeightY, InLengthZ)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawSphere(const Vector3& InLocation, const FLOAT InRadius, const Color4& InDebugColor, const Quaternion& InRotation)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SPHERE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		RequireCommands.EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InLocation, InRotation, Vector3(InRadius, InRadius, InRadius)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCone(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InRadius, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CONE - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		Vector3 Direction = InTopLocation - InBottomCenterLocation;
		FLOAT DirectionLength = Direction.Length();
		if (DirectionLength <= 1e-3f)
		{
			Direction = Vector3::XVector();
			DirectionLength = 1e-3f;
		}
		else
		{
			Direction = Direction / DirectionLength;
		}

		RequireCommands.EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InBottomCenterLocation, MakeQuaternion(Vector3::YVector(), Direction), Vector3(InRadius, DirectionLength, InRadius)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCylinder(const Vector3& InBottomCenterLocation, const Vector3& InTopLocation, const FLOAT InRadius, const Color4& InDebugColor)
	{
		const UINT32 TargetIndex = RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CYLINDER - 1u;
		TArray<Matrix4x4>& TargetTransforms = PrimitiveTransforms[TargetIndex];
		TArray<Color4>& TargetColors = PrimitiveColors[TargetIndex];

		Vector3 Direction = InTopLocation - InBottomCenterLocation;
		FLOAT DirectionLength = Direction.Length();
		if (DirectionLength <= 1e-3f)
		{
			Direction = Vector3::XVector();
			DirectionLength = 1e-3f;
		}
		else
		{
			Direction = Direction / DirectionLength;
		}

		RequireCommands.EnqueueCommand(
			[&TargetTransforms, &TargetColors,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(MakeMatrix4x4(InBottomCenterLocation, MakeQuaternion(Vector3::YVector(), Direction), Vector3(InRadius, DirectionLength, InRadius)))]()->void
			{
				TargetTransforms.Add(LocalToWorld);
				TargetColors.Add(DebugColor);
			}
		);
	}
	void RDebugWireframePrimitiveManager::DrawCustom(const EString& InCustomName, const Matrix4x4& InLocalToWorld, const Color4& InDebugColor)
	{
		TMap<EString, RDebugWireframePrimitive>& TargetPrimitives = DebugWireframeCustomPrimitives;
		TMap<EString, TArray<Matrix4x4>>& TargetTransforms = CustomPrimitiveTransforms;
		TMap<EString, TArray<Color4>>& TargetColors = CustomPrimitiveColors;

		RequireCommands.EnqueueCommand(
			[&TargetPrimitives, &TargetTransforms, &TargetColors,
			CustomName = InCustomName,
			DebugColor = InDebugColor,
			LocalToWorld = TranslateUploadMatrixType(InLocalToWorld)]()->void
			{
				if (TargetPrimitives.ContainsKey(CustomName))
				{
					if (!(TargetTransforms.ContainsKey(CustomName)))
					{
						TargetTransforms.Add(CustomName, TArray<Matrix4x4>());
					}
					if (!(TargetColors.ContainsKey(CustomName)))
					{
						TargetColors.Add(CustomName, TArray<Color4>());
					}
					TArray<Matrix4x4>* TransformContainer = TargetTransforms.FindValueAsPtr(CustomName);
					TArray<Color4>* ColorContainer = TargetColors.FindValueAsPtr(CustomName);
					TransformContainer->Add(LocalToWorld);
					ColorContainer->Add(DebugColor);
				}
			}
		);
	}
	void RDebugWireframePrimitiveManager::CreatePrimitive(RDebugWireframeType InType)
	{
		CreatePrimitiveInternal(InType);
	}
	void RDebugWireframePrimitiveManager::CreateCustomPrimitive(const EString& InCustomName, const Vector3* InPointDatas, const UINT32 InPointNum)
	{
		RDebugWireframePrimitiveManager* Manager = this;
		Vector3* PointDatas = nullptr;
		if (InPointNum > 0u)
		{
			PointDatas = new Vector3[InPointNum];
			const UINT32 MemSize = sizeof(Vector3) * InPointNum;
			::memset(PointDatas, 0, MemSize);
			::memcpy_s(PointDatas, MemSize, InPointDatas, MemSize);
		}

		RequireCreateCustomCommands.EnqueueCommand(
			[Manager, PointDatas,
			CustomName = InCustomName,
			PointNum = InPointNum
			]()->void
			{
				Manager->CreatePrimitiveInternal(RDebugWireframeType::DEBUG_WIREFRAME_CUSTOM, &CustomName, &PointNum, PointDatas);
				if (!!PointDatas)
				{
					delete[]PointDatas;
				}
			}
		);
	}
	void RDebugWireframePrimitiveManager::CreatePrimitiveInternal(RDebugWireframeType InType, const EString* InCustomName, const UINT32* InVectorNum, const Vector3* InVectorDatas)
	{
		const UINT32 TargetIndex = InType - 1u;
		BOOL32 Result = FALSE;

		switch (InType)
		{
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SINGLELINE:
		{
			const Vector3 PointListSingleLine[] =
			{
				Vector3(0.f, 0.0f, 0.f),
				Vector3(0.f, 1.0f, 0.f)
			};
			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(PointListSingleLine, PE_ARRAYSIZE(PointListSingleLine));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_PLANE:
		{
			const Vector3 PointListPlane[] =
			{
				Vector3(-0.5f, 0.5f, 0.f),
				Vector3(0.5f, 0.5f, 0.f),
				Vector3(0.5f, -0.5f, 0.f),
				Vector3(-0.5f, -0.5f, 0.f),
				Vector3(-0.5f, 0.5f, 0.f)
			};
			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(PointListPlane, PE_ARRAYSIZE(PointListPlane));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CIRCLE:
		{
			const Vector3 PointListCircle[] =
			{
				Vector3(1.f, 0.f, 0.f),
				Vector3(0.92388f, 0.382683f, 0.f),
				Vector3(0.707107f, 0.707107f, 0.f),
				Vector3(0.382683f, 0.92388f, 0.f),
				Vector3(-4.37114e-08f, 1.f, 0.f),
				Vector3(-0.382683f, 0.92388f, 0.f),
				Vector3(-0.707107f, 0.707107f, 0.f),
				Vector3(-0.92388f, 0.382683f, 0.f),
				Vector3(-1.f, -8.74228e-08f, 0.f),
				Vector3(-0.92388f, -0.382683f, 0.f),
				Vector3(-0.707107f, -0.707107f, 0.f),
				Vector3(-0.382684f, -0.92388f, 0.f),
				Vector3(1.19249e-08f, -1.f, 0.f),
				Vector3(0.382684f, -0.923879f, 0.f),
				Vector3(0.707107f, -0.707107f, 0.f),
				Vector3(0.92388f, -0.3826830f, 0.f),
				Vector3(1.f, 0.f, 0.f)
			};
			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(PointListCircle, PE_ARRAYSIZE(PointListCircle));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CUBOID:
		{
			const Vector3 CubePoints[] =
			{
				Vector3(-0.5f, 0.5f, 0.5f),
				Vector3(0.5f, 0.5f, 0.5f),
				Vector3(0.5f, 0.5f, -0.5f),
				Vector3(-0.5f, 0.5f, -0.5f),

				Vector3(-0.5f, -0.5f, 0.5f),
				Vector3(0.5f, -0.5f, 0.5f),
				Vector3(0.5f, -0.5f, -0.5f),
				Vector3(-0.5f, -0.5f, -0.5f)
			};
			const UINT32 CubeLines[] =
			{
				0u, 1u,
				1u, 2u,
				2u, 3u,
				3u, 0u,

				0u, 4u,
				1u, 5u,
				2u, 6u,
				3u, 7u,

				4u, 5u,
				5u, 6u,
				6u, 7u,
				7u, 4u
			};
			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(CubePoints, PE_ARRAYSIZE(CubePoints), CubeLines, PE_ARRAYSIZE(CubeLines));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_SPHERE:
		{
			Matrix4x4 RotY(MakeMatrix4x4(MakeQuaternion(Vector3::YVector(), EMath::DegreesToRadians(90.f))));
			Matrix4x4 RotX(MakeMatrix4x4(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.f))));

			const Vector3 SpherePoints[] =
			{
				Vector3(1.f, 0.f, 0.f),
				Vector3(0.92388f, 0.382683f, 0.f),
				Vector3(0.707107f, 0.707107f, 0.f),
				Vector3(0.382683f, 0.92388f, 0.f),
				Vector3(-4.37114e-08f, 1.f, 0.f),
				Vector3(-0.382683f, 0.92388f, 0.f),
				Vector3(-0.707107f, 0.707107f, 0.f),
				Vector3(-0.92388f, 0.382683f, 0.f),
				Vector3(-1.f, -8.74228e-08f, 0.f),
				Vector3(-0.92388f, -0.382683f, 0.f),
				Vector3(-0.707107f, -0.707107f, 0.f),
				Vector3(-0.382684f, -0.92388f, 0.f),
				Vector3(1.19249e-08f, -1.f, 0.f),
				Vector3(0.382684f, -0.923879f, 0.f),
				Vector3(0.707107f, -0.707107f, 0.f),
				Vector3(0.92388f, -0.3826830f, 0.f),
				Matrix4x4TransformPosition(RotY, SpherePoints[0]),
				Matrix4x4TransformPosition(RotY, SpherePoints[1]),
				Matrix4x4TransformPosition(RotY, SpherePoints[2]),
				Matrix4x4TransformPosition(RotY, SpherePoints[3]),
				Matrix4x4TransformPosition(RotY, SpherePoints[4]),
				Matrix4x4TransformPosition(RotY, SpherePoints[5]),
				Matrix4x4TransformPosition(RotY, SpherePoints[6]),
				Matrix4x4TransformPosition(RotY, SpherePoints[7]),
				Matrix4x4TransformPosition(RotY, SpherePoints[8]),
				Matrix4x4TransformPosition(RotY, SpherePoints[9]),
				Matrix4x4TransformPosition(RotY, SpherePoints[10]),
				Matrix4x4TransformPosition(RotY, SpherePoints[11]),
				Matrix4x4TransformPosition(RotY, SpherePoints[12]),
				Matrix4x4TransformPosition(RotY, SpherePoints[13]),
				Matrix4x4TransformPosition(RotY, SpherePoints[14]),
				Matrix4x4TransformPosition(RotY, SpherePoints[15]),
				Matrix4x4TransformPosition(RotX, SpherePoints[0]),
				Matrix4x4TransformPosition(RotX, SpherePoints[1]),
				Matrix4x4TransformPosition(RotX, SpherePoints[2]),
				Matrix4x4TransformPosition(RotX, SpherePoints[3]),
				Matrix4x4TransformPosition(RotX, SpherePoints[4]),
				Matrix4x4TransformPosition(RotX, SpherePoints[5]),
				Matrix4x4TransformPosition(RotX, SpherePoints[6]),
				Matrix4x4TransformPosition(RotX, SpherePoints[7]),
				Matrix4x4TransformPosition(RotX, SpherePoints[8]),
				Matrix4x4TransformPosition(RotX, SpherePoints[9]),
				Matrix4x4TransformPosition(RotX, SpherePoints[10]),
				Matrix4x4TransformPosition(RotX, SpherePoints[11]),
				Matrix4x4TransformPosition(RotX, SpherePoints[12]),
				Matrix4x4TransformPosition(RotX, SpherePoints[13]),
				Matrix4x4TransformPosition(RotX, SpherePoints[14]),
				Matrix4x4TransformPosition(RotX, SpherePoints[15]),
			};

#define GENERATE_SPHERE_LINES(__Index) \
				0u + (16u * __Index), 1u + (16u * __Index),\
				1u + (16u * __Index), 2u + (16u * __Index),\
				2u + (16u * __Index), 3u + (16u * __Index),\
				3u + (16u * __Index), 4u + (16u * __Index),\
				4u + (16u * __Index), 5u + (16u * __Index),\
				5u + (16u * __Index), 6u + (16u * __Index),\
				6u + (16u * __Index), 7u + (16u * __Index),\
				7u + (16u * __Index), 8u + (16u * __Index),\
				8u + (16u * __Index), 9u + (16u * __Index),\
				9u + (16u * __Index), 10u + (16u * __Index),\
				10u + (16u * __Index), 11u + (16u * __Index),\
				11u + (16u * __Index), 12u + (16u * __Index),\
				12u + (16u * __Index), 13u + (16u * __Index),\
				13u + (16u * __Index), 14u + (16u * __Index),\
				14u + (16u * __Index), 15u + (16u * __Index),\
				15u + (16u * __Index), 0u + (16u * __Index)\

			const UINT32 SphereLines[] =
			{
				GENERATE_SPHERE_LINES(0u),
				GENERATE_SPHERE_LINES(1u),
				GENERATE_SPHERE_LINES(2u)
			};

#undef GENERATE_SPHERE_LINES

			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(SpherePoints, PE_ARRAYSIZE(SpherePoints), SphereLines, PE_ARRAYSIZE(SphereLines));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CONE:
		{
			Matrix4x4 RotX(MakeMatrix4x4(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.f))));
			const Vector3 ConePoints[] =
			{
				Matrix4x4TransformPosition(RotX, Vector3(1.f, 0.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.92388f, 0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.707107f, 0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.382683f, 0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-4.37114e-08f, 1.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.382683f, 0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.707107f, 0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.92388f, 0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-1.f, -8.74228e-08f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.92388f, -0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.707107f, -0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.382684f, -0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(1.19249e-08f, -1.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.382684f, -0.923879f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.707107f, -0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.92388f, -0.3826830f, 0.f)),

				Vector3(0.f, 0.f, 0.f),
				Vector3(0.f, 1.f, 0.f)
			};
			const UINT32 ConeLines[] =
			{
				0u, 1u,
				1u, 2u,
				2u, 3u,
				3u, 4u,
				4u, 5u,
				5u, 6u,
				6u, 7u,
				7u, 8u,
				8u, 9u,
				9u, 10,
				10u, 11u,
				11u, 12u,
				12u, 13u,
				13u, 14u,
				14u, 15u,
				15u, 0u,

				16u, 17u
			};
			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(ConePoints, PE_ARRAYSIZE(ConePoints), ConeLines, PE_ARRAYSIZE(ConeLines));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_ENGINE_CYLINDER:
		{
			Matrix4x4 RotX(MakeMatrix4x4(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.f))));
			const Vector3 CylinderPoints[] =
			{
				Matrix4x4TransformPosition(RotX, Vector3(1.f, 0.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.92388f, 0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.707107f, 0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.382683f, 0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-4.37114e-08f, 1.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.382683f, 0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.707107f, 0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.92388f, 0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-1.f, -8.74228e-08f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.92388f, -0.382683f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.707107f, -0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(-0.382684f, -0.92388f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(1.19249e-08f, -1.f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.382684f, -0.923879f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.707107f, -0.707107f, 0.f)),
				Matrix4x4TransformPosition(RotX, Vector3(0.92388f, -0.3826830f, 0.f)),

				CylinderPoints[0] + (Vector3::YVector() * 1.0f),
				CylinderPoints[1] + (Vector3::YVector() * 1.0f),
				CylinderPoints[2] + (Vector3::YVector() * 1.0f),
				CylinderPoints[3] + (Vector3::YVector() * 1.0f),
				CylinderPoints[4] + (Vector3::YVector() * 1.0f),
				CylinderPoints[5] + (Vector3::YVector() * 1.0f),
				CylinderPoints[6] + (Vector3::YVector() * 1.0f),
				CylinderPoints[7] + (Vector3::YVector() * 1.0f),
				CylinderPoints[8] + (Vector3::YVector() * 1.0f),
				CylinderPoints[9] + (Vector3::YVector() * 1.0f),
				CylinderPoints[10] + (Vector3::YVector() * 1.0f),
				CylinderPoints[11] + (Vector3::YVector() * 1.0f),
				CylinderPoints[12] + (Vector3::YVector() * 1.0f),
				CylinderPoints[13] + (Vector3::YVector() * 1.0f),
				CylinderPoints[14] + (Vector3::YVector() * 1.0f),
				CylinderPoints[15] + (Vector3::YVector() * 1.0f),

				Vector3(0.f, 0.f, 0.f),
				Vector3(0.f, 1.f, 0.f)
			};
			const UINT32 CylinderLines[] =
			{
				0u, 1u,
				1u, 2u,
				2u, 3u,
				3u, 4u,
				4u, 5u,
				5u, 6u,
				6u, 7u,
				7u, 8u,
				8u, 9u,
				9u, 10,
				10u, 11u,
				11u, 12u,
				12u, 13u,
				13u, 14u,
				14u, 15u,
				15u, 0u,

				0u + 16u, 1u + 16u,
				1u + 16u, 2u + 16u,
				2u + 16u, 3u + 16u,
				3u + 16u, 4u + 16u,
				4u + 16u, 5u + 16u,
				5u + 16u, 6u + 16u,
				6u + 16u, 7u + 16u,
				7u + 16u, 8u + 16u,
				8u + 16u, 9u + 16u,
				9u + 16u, 10 + 16u,
				10u + 16u, 11u + 16u,
				11u + 16u, 12u + 16u,
				12u + 16u, 13u + 16u,
				13u + 16u, 14u + 16u,
				14u + 16u, 15u + 16u,
				15u + 16u, 0u + 16u,

				32u, 33u
			};
			Result = DebugWireframePrimitives[TargetIndex].InitPrimitive(CylinderPoints, PE_ARRAYSIZE(CylinderPoints), CylinderLines, PE_ARRAYSIZE(CylinderLines));
		}
		break;
		case RDebugWireframeType::DEBUG_WIREFRAME_CUSTOM:
		{
			if ((!!InCustomName) && (!!InVectorNum) && (!!InVectorDatas) && ((*InCustomName).Length() > 1u) && (*InVectorNum > 0u))
			{
				const EString& CustomName = (*InCustomName);
				if (!(DebugWireframeCustomPrimitives.ContainsKey(CustomName)))
				{
					DebugWireframeCustomPrimitives.Add(CustomName, RDebugWireframePrimitive());
					RDebugWireframePrimitive* New = DebugWireframeCustomPrimitives.FindValueAsPtr(CustomName);
					Result = New->InitPrimitive(InVectorDatas, (*InVectorNum));
				}
			}
		}
		break;
		default:
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Error type for debug wireframe primitive."));
		}
		break;
		}

		{
			EString Info("Init debug wireframe primitive.");
			Info = Info + "Type : " + ToString(static_cast<UINT32>(InType));
			if (InCustomName)
			{
				Info = Info + " Custom name : " + (*InCustomName);
			}
			if (Result)
			{
				Info = Info + " success.";
				PE_LOG_LOG(Info);
			}
			else
			{
				Info = Info + " failed.";
				PE_LOG_WARN(Info);
			}
		}
	}
#endif

};