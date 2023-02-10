#include "../Headers/CBezierGrass.h"
#include "../../EngineRender/RenderBase/Headers/CMeshRendererComponent.h"
#include "../../EngineGame/Headers/CScreenPolygon2D.h"

CBezierGrass::CBezierGrass()
{
#ifdef _DEVELOPMENT_EDITOR
	m_IsShowGrassDebugRS	= FALSE;
	m_GrassDebugRS			= nullptr;
	m_DebugGrassRot			= 0.f;
	m_IsShowDebugWnd		= FALSE;
	m_DebugWnd				= nullptr;
	m_DebugBufferSizeX		= 800u;
	m_DebugBufferSizeY		= 600u;
	m_DebugCBufferData.Parameter		= DirectX::XMINT4(static_cast<INT>(m_DebugBufferSizeY), 0, 0, 0);
	m_DebugCBufferData.StartEndPoint	= DirectX::XMFLOAT4(0.f, 0.f, 1.f, 1.f);
	m_DebugCBufferData.ControlPoint		= DirectX::XMFLOAT4(0.f, 0.5f, 0.5f, 1.f);
#endif

	m_GrassPropertyData.SplitNum	= 0u;
	m_GrassPropertyData.StepLength	= 0.f;
	m_GrassPropertyData.LeafWidth	= 0.f;
	m_GrassPropertyData.RootColor	= CustomStruct::CColor(0.f, 0.f, 0.f, 1.f);
	m_GrassPropertyData.TipColor	= CustomStruct::CColor(0.f, 0.f, 0.f, 1.f);
	m_GrassPropertyCBuffer			= nullptr;

	m_GrassInstanceNumInBuffer = 0u;
	if (m_GrassInstanceData.size() > 0)
	{
		m_GrassInstanceData.clear();
	}
	m_GrassInstanceBuffer.Release();
	m_GrassInputLayout		= nullptr;
	m_GrassVertexShader		= nullptr;
	m_GrassPixelShader		= nullptr;
	m_GrassIndexNum			= 0u;
	m_GrassIndexBuffer		= nullptr;
	m_GrassRS				= nullptr;
	m_PipelineRS			= nullptr;
}
CBezierGrass::~CBezierGrass()
{
	if (m_GrassInputLayout)
	{
		m_GrassInputLayout->Release();
		m_GrassInputLayout = nullptr;
	}
	if (m_GrassVertexShader)
	{
		m_GrassVertexShader->Release();
		m_GrassVertexShader = nullptr;
	}
	if (m_GrassPixelShader)
	{
		m_GrassPixelShader->Release();
		m_GrassPixelShader = nullptr;
	}
	m_GrassInstanceBuffer.Release();
	if (m_GrassInstanceData.size() > 0)
	{
		m_GrassInstanceData.clear();
	}
	if (m_GrassPropertyCBuffer)
	{
		m_GrassPropertyCBuffer->Release();
		m_GrassPropertyCBuffer = nullptr;
	}
	if (m_GrassRS)
	{
		m_GrassRS->Release();
		m_GrassRS = nullptr;
	}
	if (m_PipelineRS)
	{
		m_PipelineRS->Release();
		m_PipelineRS = nullptr;
	}
#ifdef _DEVELOPMENT_EDITOR
	if (m_DebugWnd != nullptr)
	{
		delete m_DebugWnd;
		m_DebugWnd = nullptr;
	}
	if (m_GrassDebugRS)
	{
		m_GrassDebugRS->Release();
		m_GrassDebugRS = nullptr;
	}
#endif
}
void CBezierGrass::Init()
{
	if (!m_GrassRS)
	{
		CRenderDevice::CreateRasterizerState(m_GrassRS, CustomStruct::CRenderRasterizerState(CustomStruct::CRenderCullMode::CULL_NONE, CustomStruct::CRenderFillMode::FILL_SOLID));
	}
	if (!m_PipelineRS)
	{
		CRenderDevice::CreateRasterizerState(m_PipelineRS, CustomStruct::CRenderRasterizerState(CustomStruct::CRenderCullMode::CULL_BACK, CustomStruct::CRenderFillMode::FILL_SOLID));
	}

	if (!m_GrassVertexShader || !m_GrassInputLayout)
	{
		if (m_GrassVertexShader)
		{
			m_GrassVertexShader->Release();
			m_GrassVertexShader = nullptr;
		}
		if (m_GrassInputLayout)
		{
			m_GrassInputLayout->Release();
			m_GrassInputLayout = nullptr;
		}
		CRenderDevice::LoadVertexShader("./Engine/Assets/EngineShaders/BezierGrassVS.cso", m_GrassVertexShader, m_GrassInputLayout, NULL, 0u);
	}
	if (!m_GrassPixelShader)
	{
		CRenderDevice::LoadPixelShader("./Engine/Assets/EngineShaders/BezierGrassPS.cso", m_GrassPixelShader);
	}

#if 1
	ReCreateGrassInstanceData(CustomType::Vector3::Zero(), 40.f, 10.f, 1000.f, 1000.f, 128u, 128u);
#else
	{
		if (m_GrassInstanceData.size() > 0)
		{
			m_GrassInstanceData.clear();
		}
		m_GrassInstanceData.resize(1u);

		m_GrassInstanceData[0].Origin = CustomType::Vector3(0.f, 0.f, 0.f);
		m_GrassInstanceData[0].Direction = CustomType::Vector3(1.f, 0.f, 0.f);
		m_GrassInstanceData[0].Tip = CustomType::Vector2(0.f, 5.f);
		m_GrassInstanceData[0].Tilt = 0.f;
		m_GrassInstanceData[0].Bend = 0.f;
	}
#endif

	if (!m_GrassPropertyCBuffer)
	{
		CRenderDevice::CreateBuffer(m_GrassPropertyCBuffer, CustomStruct::CRenderBufferDesc(sizeof(GrassPropertyBuffer), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	}
	m_GrassPropertyData.SplitNum	= 6u;
	m_GrassPropertyData.StepLength	= 1.f / static_cast<FLOAT>(m_GrassPropertyData.SplitNum);
	m_GrassPropertyData.LeafWidth	= 0.5f;
	m_GrassPropertyData.RootColor	= CustomStruct::CColor(35.f / 255.f, 30.f / 255.f, 6.f / 255.f, 1.f);
	m_GrassPropertyData.TipColor	= CustomStruct::CColor(15.f / 255.f, 88.f / 255.f, 5.f / 255.f, 1.f);

#ifdef _DEVELOPMENT_EDITOR
	if (!m_GrassDebugRS)
	{
		CRenderDevice::CreateRasterizerState(m_GrassDebugRS, CustomStruct::CRenderRasterizerState(CustomStruct::CRenderCullMode::CULL_NONE, CustomStruct::CRenderFillMode::FILL_WIREFRAME));
	}

	{
		//m_DebugGrassRot = 0.f;
		//FLOAT tempSin, tempCos;
		//CustomType::CMath::SinCos(tempSin, tempCos, m_DebugGrassRot * CustomType::CMath::GetDegToRad());
		//m_GrassInstanceData[0].Direction = CustomType::Vector3::Normalize(CustomType::Vector3(tempSin, 0.f, tempCos));
	}
	FLOAT startX = 400.f;
	FLOAT startY = 150.f;
	if (m_DebugWnd == nullptr)
	{
		m_DebugWnd = new CScreenPolygon2D(TRUE, nullptr, ENGINE_SHADER_SCREEN_POLYGON_2D_PS, CustomType::Vector4(startX, startY, startX + static_cast<FLOAT>(m_DebugBufferSizeX), startY + static_cast<FLOAT>(m_DebugBufferSizeY)));
		m_DebugWnd->Init();
	}
	CRenderDevice::CreateBuffer(m_DebugCBuffer, CustomStruct::CRenderBufferDesc(sizeof(DebugCBufferData), CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, sizeof(FLOAT)));
	CRenderDevice::CreateRenderTexture2D(m_DebugTexBuffer,
		CustomStruct::CRenderTextureDesc(
			m_DebugBufferSizeX, m_DebugBufferSizeY,
			CustomStruct::CRenderBindFlag::BIND_SRV_UAV,
			CustomStruct::CRenderFormat::FORMAT_R32_UINT));
	CRenderDevice::CreateRenderTexture2D(m_DebugScreenBuffer,
		CustomStruct::CRenderTextureDesc(
			m_DebugBufferSizeX, m_DebugBufferSizeY,
			CustomStruct::CRenderBindFlag::BIND_SRV_UAV,
			CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM));
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/TestBezierGrassDebugCS.cso", m_DebugCS);
	CRenderDevice::LoadComputeShader("./Engine/Assets/EngineShaders/TestBezierGrassDebugScreenCS.cso", m_DebugScreenCS);
#endif
}
void CBezierGrass::Update()
{
#ifdef _DEVELOPMENT_EDITOR
	{
		INT	grassSplitNum			= static_cast<INT>(m_GrassPropertyData.SplitNum);
		FLOAT grassLeafWidth		= m_GrassPropertyData.LeafWidth;
		FLOAT grassRootColor[3]		= { m_GrassPropertyData.RootColor.r, m_GrassPropertyData.RootColor.g, m_GrassPropertyData.RootColor.b };
		FLOAT grassTipColor[3]		= { m_GrassPropertyData.TipColor.r, m_GrassPropertyData.TipColor.g, m_GrassPropertyData.TipColor.b };

		FLOAT grassOrigin[3]		= { m_GrassInstanceData[0].Origin.X(), m_GrassInstanceData[0].Origin.Y(), m_GrassInstanceData[0].Origin.Z() };
		FLOAT grassTip[2]			= { m_GrassInstanceData[0].Tip.X(), m_GrassInstanceData[0].Tip.Y() };
		FLOAT grassTilt				= m_GrassInstanceData[0].Tilt;
		FLOAT grassBend				= m_GrassInstanceData[0].Bend;

		bool isShowGrassDebugRS		= m_IsShowGrassDebugRS;
		bool isShowDebugWnd			= m_IsShowDebugWnd;
		FLOAT grassAngle			= m_DebugGrassRot;
		INT threadNum				= m_DebugCBufferData.Parameter.x;
		FLOAT endPoint[2]			= { m_DebugCBufferData.StartEndPoint.z, m_DebugCBufferData.StartEndPoint.w };
		FLOAT controlStartPoint[2]	= { m_DebugCBufferData.ControlPoint.x, m_DebugCBufferData.ControlPoint.y };
		FLOAT controlEndPoint[2]	= { m_DebugCBufferData.ControlPoint.z, m_DebugCBufferData.ControlPoint.w };

		ImGui::Begin("ControlBezier");

		ImGui::Checkbox("IsShowGrassDebugWireframe", &isShowGrassDebugRS);
		ImGui::SliderInt("GrassSplitNum", &grassSplitNum, 2, 15, "%d", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat("GrassLeafWidth", &grassLeafWidth, 0.001f, 30.f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		ImGui::ColorEdit3("GrassRootColor", grassRootColor);
		ImGui::ColorEdit3("GrassTipColor", grassTipColor);

		//ImGui::SliderFloat3("GrassOrigin", grassOrigin, -100.f, 100.f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		//ImGui::SliderFloat2("GrassTip", grassTip, 0.f, 200.f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		//ImGui::SliderAngle("GrassDirection", &grassAngle, -360.f, 360.f, "%.0f deg", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		//ImGui::SliderAngle("GrassTilt", &grassTilt, 0.f, 90.f, "%.0f deg", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		//ImGui::SliderFloat("GrassBend", &grassBend, 0.f, 1.f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);

		ImGui::Checkbox("IsShowDebugWindow", &isShowDebugWnd);
		ImGui::SliderInt("ThreadCount", &threadNum, 1, 2048, "%d", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat2("EndPoint", endPoint, 0.f, 1.f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat2("StartControlPoint", controlStartPoint, 0.f, 1.f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		ImGui::SliderFloat2("EndControlPoint", controlEndPoint, 0.f, 1.f, "%.3f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);

		ImGui::End();

		m_IsShowGrassDebugRS				= isShowGrassDebugRS;
		m_IsShowDebugWnd					= isShowDebugWnd;
		m_DebugCBufferData.Parameter		= DirectX::XMINT4(threadNum, 0, 0, 0);
		m_DebugCBufferData.StartEndPoint	= DirectX::XMFLOAT4(0.f, 0.f, endPoint[0], endPoint[1]);
		m_DebugCBufferData.ControlPoint		= DirectX::XMFLOAT4(controlStartPoint[0], controlStartPoint[1], controlEndPoint[0], controlEndPoint[1]);
		m_DebugGrassRot						= grassAngle;
		FLOAT tempSin, tempCos;
		CustomType::CMath::SinCos(tempSin, tempCos, m_DebugGrassRot);

		//m_GrassInstanceData[0].Origin		= CustomType::Vector3(grassOrigin[0], grassOrigin[1], grassOrigin[2]);
		//m_GrassInstanceData[0].Direction	= CustomType::Vector3::Normalize(CustomType::Vector3(tempCos, m_GrassInstanceData[0].Direction.Y(), tempSin));
		//m_GrassInstanceData[0].Tip			= CustomType::Vector2(grassTip[0], grassTip[1]);
		//m_GrassInstanceData[0].Tilt			= grassTilt;
		//m_GrassInstanceData[0].Bend			= grassBend;


		m_GrassPropertyData.SplitNum	= static_cast<UINT>(grassSplitNum);
		m_GrassPropertyData.StepLength	= 1.f / static_cast<FLOAT>(m_GrassPropertyData.SplitNum);
		m_GrassPropertyData.LeafWidth	= grassLeafWidth;
		m_GrassPropertyData.RootColor	= CustomStruct::CColor(grassRootColor[0], grassRootColor[1], grassRootColor[2], 1.f);
		m_GrassPropertyData.TipColor	= CustomStruct::CColor(grassTipColor[0], grassTipColor[1], grassTipColor[2], 1.f);
	}

	if (m_IsShowDebugWnd)
	{
		CRenderDevice::UploadBuffer(m_DebugCBuffer, static_cast<const void*>(&m_DebugCBufferData));

		CRenderDevice::ClearUnorderedAccessViewUint(m_DebugTexBuffer.UnorderedAccessView);
		CRenderDevice::ClearUnorderedAccessViewFloat(m_DebugScreenBuffer.UnorderedAccessView);

		UINT threadSize = static_cast<UINT>(((m_DebugCBufferData.Parameter.x + 7) / 8) * 8);
		CRenderDevice::BindCSConstantBuffer(m_DebugCBuffer, 2u);

		CRenderDevice::SetCSShader(m_DebugCS);
		CRenderDevice::BindCSUnorderedAccessView(m_DebugTexBuffer.UnorderedAccessView, 0u);
		CRenderDevice::Dispatch(threadSize / 8u, 1u, 1u);

		CRenderDevice::SetCSShader(m_DebugScreenCS);
		CRenderDevice::BindCSUnorderedAccessView(m_DebugScreenBuffer.UnorderedAccessView, 0u);
		CRenderDevice::BindCSShaderResourceView(m_DebugTexBuffer.ShaderResourceView, 5u);
		CRenderDevice::Dispatch((m_DebugBufferSizeX + 7u) / 8u, (m_DebugBufferSizeY + 7u) / 8u, 1u);
		CRenderDevice::BindNoCSUnorderedAccessView(0u);
		CRenderDevice::SetNoCSShader();
	}
#endif
	{
		UploadGrassPropertyConstantBuffer();
		const UINT numInstance = static_cast<UINT>(m_GrassInstanceData.size());
		if (m_GrassInstanceNumInBuffer != numInstance)
		{
			m_GrassInstanceNumInBuffer = numInstance;
			ReCreateGrassInstanceConstantBuffer();
		}
		UploadGrassInstanceConstantBuffer();
	}
}
void CBezierGrass::Render()
{
#ifdef _DEVELOPMENT_EDITOR
	if (m_IsShowGrassDebugRS)
	{
		CRenderDevice::SetRasterizerState(m_GrassDebugRS);
	}
	else
#endif
	{
		CRenderDevice::SetRasterizerState(m_GrassRS);
	}

	const UINT indexNum = (m_GrassPropertyData.SplitNum * 2u - 1u) * 3u;
	ReCreateGrassIndexBuffer(indexNum);

	CRenderDevice::SetNoInputLayout();
	CRenderDevice::SetVSShader(m_GrassVertexShader);
	CRenderDevice::SetPSShader(m_GrassPixelShader);
	CRenderDevice::SetNoVertexBuffer();
	CRenderDevice::SetIndexBuffer(m_GrassIndexBuffer, 0u, CustomStruct::CRenderFormat::FORMAT_R16_UINT);
	CRenderDevice::BindVSConstantBuffer(m_GrassPropertyCBuffer, 2u);
	CRenderDevice::BindPSConstantBuffer(m_GrassPropertyCBuffer, 2u);
	//CRenderDevice::BindVSConstantBuffer(m_GrassInstanceBuffer.Buffer, 3u);
	//CRenderDevice::DrawIndexed(indexNum);
	CRenderDevice::BindVSShaderResourceView(m_GrassInstanceBuffer.ShaderResourceView, 5u);
	CRenderDevice::DrawIndexedInstance(m_GrassInstanceNumInBuffer, indexNum);

	CRenderDevice::SetRasterizerState(m_PipelineRS);
}
void CBezierGrass::Uninit()
{
	m_GrassInstanceNumInBuffer = 0u;
	m_GrassInstanceBuffer.Release();
	if (m_GrassInputLayout)
	{
		m_GrassInputLayout->Release();
		m_GrassInputLayout = nullptr;
	}
	if (m_GrassVertexShader)
	{
		m_GrassVertexShader->Release();
		m_GrassVertexShader = nullptr;
	}
	if (m_GrassPixelShader)
	{
		m_GrassPixelShader->Release();
		m_GrassPixelShader = nullptr;
	}
#ifdef _DEVELOPMENT_EDITOR
	if (m_DebugWnd != nullptr)
	{
		delete m_DebugWnd;
		m_DebugWnd = nullptr;
	}
#endif
}
void CBezierGrass::ReCreateGrassInstanceData(const CustomType::Vector3& origin, const FLOAT& BaseHeight, const FLOAT& offsetHeight, const FLOAT& lengthX, const FLOAT& lengthZ, const UINT& numX, const UINT& numZ)
{
	if (numX == 0u || numZ == 0u || lengthX < 1e-3f || lengthZ < 1e-3f || BaseHeight < 1e-3f || offsetHeight < 0.f)
	{
		return;
	}

	auto _RandomFloat = [](const FLOAT& x, const FLOAT& y)->FLOAT
	{
		return (CustomType::CMath::Frac(CustomType::CMath::Sin(x * 12.9898f + y * 78.233f) * 43758.5453123f));
	};
	auto _RandomVector2 = [](const FLOAT& x, const FLOAT& y, FLOAT& nx, FLOAT& ny)->void
	{
		nx = CustomType::CMath::Frac(CustomType::CMath::Sin(x * 127.1f + y * 311.7f) * 43758.5453123f);
		ny = CustomType::CMath::Frac(CustomType::CMath::Sin(x * 269.5f + y * 183.3f) * 43758.5453123f);
	};

	UINT grassSize = CustomType::CMath::Min(CBezierGrass::_GrassInstancePerGroupMaxNum, numX * numZ);
	if (m_GrassInstanceData.size() != static_cast<size_t>(grassSize))
	{
		m_GrassInstanceData.resize(static_cast<size_t>(grassSize));
	}

	{
		CustomType::Vector3 tempOrigin(origin);
		FLOAT deltaX = lengthX / static_cast<FLOAT>(numX), deltaZ = lengthZ / static_cast<FLOAT>(numZ);
		FLOAT offsetXStart = -lengthX * 0.5f + deltaX * 0.5f, offsetXEnd = lengthX * 0.5f - deltaX * 0.5f;
		FLOAT offsetZStart = -lengthZ * 0.5f + deltaZ * 0.5f, offsetZEnd = lengthZ * 0.5f - deltaZ * 0.5f;
		for (UINT z = 0u; z < numZ; z++)
		{
			for (UINT x = 0u; x < numX; x++)
			{
				UINT index = z * numX + x;
				if (index >= grassSize)
				{
					//TODO return the over size part.
					continue;
				}
				FLOAT tx = static_cast<FLOAT>(x); FLOAT tz = static_cast<FLOAT>(z);
				if (numX > 1) { tx = tx / static_cast<FLOAT>(numX - 1u); }
				if (numZ > 1) { tz = tz / static_cast<FLOAT>(numZ - 1u); }
				FLOAT nx, nz;
				_RandomVector2(tx, tz, nx, nz);
				CustomType::Vector3 tempOffset(offsetXStart * (1.f - tx) + offsetXEnd * tx + nx * deltaX * 0.5f, 0.f, offsetZStart * (1.f - tz) + offsetZEnd * tz + nz * deltaZ * 0.5f);
				GrassInstanceInfo& tempGrassData = m_GrassInstanceData[index];
				tempGrassData.Origin = tempOrigin + tempOffset;
				tempGrassData.Direction = CustomType::Vector3(1.f, 0.f, 0.f);
				tempGrassData.Tip = CustomType::Vector2(0.f, BaseHeight + offsetHeight + _RandomFloat(tx, tz) * offsetHeight);
				tempGrassData.Tilt = (0.3f + 0.3f * _RandomFloat(tx * 5.f, tz * 5.f)) * 90.f * CustomType::CMath::GetDegToRad();
				tempGrassData.Bend = 0.2f + 0.6f * _RandomFloat(tx * 10.f, tz * 10.f);
			}
		}
	}
}
void CBezierGrass::ReCreateGrassIndexBuffer(const UINT& num)
{
	if (m_GrassIndexNum != num || !m_GrassIndexBuffer)
	{
		m_GrassIndexNum = num;
		if (m_GrassIndexBuffer)
		{
			m_GrassIndexBuffer->Release();
			m_GrassIndexBuffer = nullptr;
		}
		USHORT* tempIndices = new USHORT[num];
		const USHORT tempNumIndices = static_cast<USHORT>(num);
		for (UINT i = 0u; i < tempNumIndices; i++)
		{
			tempIndices[i] = i;
		}
		CRenderDevice::CreateBuffer(m_GrassIndexBuffer,
			CustomStruct::CRenderBufferDesc(sizeof(USHORT) * num, CustomStruct::CRenderBindFlag::BIND_INDEX_BUFFER, 0u),
			&CustomStruct::CRenderSubresourceData(static_cast<const void*>(tempIndices), 0u, 0u));
		delete[]tempIndices;
	}
}
void CBezierGrass::ReCreateGrassInstanceConstantBuffer()
{
	m_GrassInstanceBuffer.Release();
	CRenderDevice::CreateStructuredBuffer(m_GrassInstanceBuffer, CustomStruct::CRenderStructuredBufferDesc(sizeof(GrassInstanceBuffer), CustomType::CMath::Min(_GrassInstancePerGroupMaxNum, static_cast<UINT>(m_GrassInstanceData.size()))));
}
void CBezierGrass::UploadGrassInstanceConstantBuffer()
{
	const UINT numGrassInstance = CustomType::CMath::Min(_GrassInstancePerGroupMaxNum, static_cast<UINT>(m_GrassInstanceData.size()));
	if (numGrassInstance > 0u)
	{
		GrassInstanceBuffer* tempBufferData = new GrassInstanceBuffer[numGrassInstance];
		for (UINT i = 0u; i < numGrassInstance; i++)
		{
			TranslateGrassInstanceInfoToBuffer(tempBufferData[i], m_GrassInstanceData[i]);
		}
		CRenderDevice::UploadBuffer(m_GrassInstanceBuffer.Buffer, static_cast<const void*>(tempBufferData), sizeof(GrassInstanceBuffer) * numGrassInstance, sizeof(GrassInstanceBuffer) * numGrassInstance);
		delete[]tempBufferData;
	}
}
void CBezierGrass::UploadGrassPropertyConstantBuffer()
{
	GrassPropertyBuffer tempBuffer;
	TranslateGrassPropertyInfoToBuffer(tempBuffer, m_GrassPropertyData);
#ifdef _DEVELOPMENT_EDITOR
	if (m_IsShowGrassDebugRS)
	{
		tempBuffer.Params1 = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
		tempBuffer.Params2 = DirectX::XMFLOAT4(0.f, 0.f, 0.f, 1.f);
	}
#endif
	CRenderDevice::UploadBuffer(m_GrassPropertyCBuffer, static_cast<const void*>(&tempBuffer));
}
void CBezierGrass::TranslateGrassInstanceInfoToBuffer(GrassInstanceBuffer& output, const GrassInstanceInfo& input)
{
	output.Params0		= DirectX::XMFLOAT4(input.Origin.X(), input.Origin.Y(), input.Origin.Z(), input.Tilt);
	output.Params1		= DirectX::XMFLOAT4(input.Direction.X(), input.Direction.Y(), input.Direction.Z(), input.Bend);
	output.Params2		= DirectX::XMFLOAT2(input.Tip.X(), input.Tip.Y());
}
void CBezierGrass::TranslateGrassInstanceBufferToInfo(GrassInstanceInfo& output, const GrassInstanceBuffer& input)
{
	output.Origin		= CustomType::Vector3(input.Params0.x, input.Params0.y, input.Params0.z);
	output.Direction	= CustomType::Vector3(input.Params1.x, input.Params1.y, input.Params1.z);
	output.Tip			= CustomType::Vector2(input.Params2);
	output.Tilt			= input.Params0.w;
	output.Bend			= input.Params1.w;
}
void CBezierGrass::TranslateGrassPropertyInfoToBuffer(GrassPropertyBuffer& output, const GrassPropertyInfo& input)
{
	output.Params0		= DirectX::XMFLOAT4(static_cast<FLOAT>(input.SplitNum), input.StepLength, input.LeafWidth, 0.f);
	output.Params1		= DirectX::XMFLOAT4(input.RootColor.r, input.RootColor.g, input.RootColor.b, input.RootColor.a);
	output.Params2		= DirectX::XMFLOAT4(input.TipColor.r, input.TipColor.g, input.TipColor.b, input.TipColor.a);
}
void CBezierGrass::TranslateGrassPropertyBufferToInfo(GrassPropertyInfo& output, const GrassPropertyBuffer& input)
{
	output.SplitNum		= static_cast<UINT>(input.Params0.x);
	output.StepLength	= input.Params0.y;
	output.LeafWidth	= input.Params0.z;
	output.RootColor	= CustomStruct::CColor(input.Params1.x, input.Params1.y, input.Params1.z, input.Params1.w);
	output.TipColor		= CustomStruct::CColor(input.Params2.x, input.Params2.y, input.Params2.z, input.Params2.w);
}
#ifdef _DEVELOPMENT_EDITOR
void CBezierGrass::DrawDebug()
{
	if (m_IsShowDebugWnd)
	{
		if (m_DebugWnd != nullptr)
		{
			CRenderDevice::BindPSShaderResourceView(m_DebugScreenBuffer.ShaderResourceView, ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT);
			const CMeshRendererComponent* meshRenderer = m_DebugWnd->GetMeshRendererComponent<CMeshRendererComponent>();
			if (meshRenderer)
			{
				meshRenderer->Draw();
			}
		}
	}
}
#endif