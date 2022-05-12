///****************************************************************************************
//
//   Copyright (C) 2015 Autodesk, Inc.
//   All rights reserved.
//
//   Use of this software is subject to the terms of the Autodesk license agreement
//   provided at the time of installation or download, or which otherwise accompanies
//   this software in either electronic or hard copy form.
//
//****************************************************************************************/
//
//
//
//
//
//
//#include "../../Headers/Base/CReadFBX.h"
//
//
//
//
//
//
//
//namespace
//{
//
//	const FLOAT		ANGLE_TO_RADIAN			= 3.1415926f / 180.f;
//	const FLOAT		BLACK_COLOR[]			= { 0.0f, 0.0f, 0.0f, 1.0f };
//	const FLOAT		GREEN_COLOR[]			= { 0.0f, 1.0f, 0.0f, 1.0f };
//	const FLOAT		WHITE_COLOR[]			= { 1.0f, 1.0f, 1.0f, 1.0f };
//	const FLOAT		WIREFRAME_COLOR[]		= { 0.5f, 0.5f, 0.5f, 1.0f };
//
//	const INT		TRIANGLE_VERTEX_COUNT	= 3;
//
//	// Four floats for every position.
//	const INT VERTEX_STRIDE			= 4;
//	// Three floats for every normal.
//	const INT NORMAL_STRIDE			= 3;
//	// Two floats for every UV.
//	const INT UV_STRIDE				= 2;
//
//	const FLOAT	DEFAULT_LIGHT_POSITION[]			= { 0.0f, 0.0f, 0.0f, 1.0f };
//	const FLOAT	DEFAULT_DIRECTION_LIGHT_POSITION[]	= { 0.0f, 0.0f, 1.0f, 0.0f };
//	const FLOAT	DEFAULT_SPOT_LIGHT_DIRECTION[]		= { 0.0f, 0.0f, -1.0f };
//	const FLOAT	DEFAULT_LIGHT_COLOR[]				= { 1.0f, 1.0f, 1.0f, 1.0f };
//	const FLOAT	DEFAULT_LIGHT_SPOT_CUTOFF			= 180.0f;
//
//	// Get specific property value and connected texture if any.
//	// Value = Property value * Factor property value (if no factor property, multiply by 1).
//	FbxDouble3 GetMaterialProperty(	const FbxSurfaceMaterial*	pMaterial,
//									const CHAR*					pPropertyName,
//									const CHAR*					pFactorPropertyName,
//									vector<string>&				vectorTextureName)
//	{
//
//		FbxDouble3 lResult(0, 0, 0);
//		const FbxProperty lProperty = pMaterial->FindProperty(pPropertyName);
//		const FbxProperty lFactorProperty = pMaterial->FindProperty(pFactorPropertyName);
//		if (lProperty.IsValid() && lFactorProperty.IsValid())
//		{
//
//			lResult = lProperty.Get<FbxDouble3>();
//			DOUBLE lFactor = lFactorProperty.Get<FbxDouble>();
//			if (lFactor != 1)
//			{
//
//				lResult[0] *= lFactor;
//				lResult[1] *= lFactor;
//				lResult[2] *= lFactor;
//
//			}
//
//		}
//
//		if (lProperty.IsValid())
//		{
//
//			const INT lTextureCount = lProperty.GetSrcObjectCount<FbxFileTexture>();
//			if (lTextureCount)
//			{
//				for (INT i = 0; i < lTextureCount; ++i)
//				{
//					const FbxFileTexture* lTexture = lProperty.GetSrcObject<FbxFileTexture>(i);
//					if (lTexture)
//					{
//						string lTextureName = lTexture->GetFileName();
//						vectorTextureName.push_back(lTextureName);
//					}
//				}
//			}
//
//		}
//
//		return lResult;
//
//	}
//
//}
//
//
//
//VBOMesh::VBOMesh() : mHasNormal(false), mHasUV(false), mAllByControlPoint(true)
//{
//
//	m_VertexBuffer = NULL;
//	m_IndexBuffer = NULL;
//
//}
//
//VBOMesh::~VBOMesh()
//{
//
//	if (m_VertexBuffer)
//		m_VertexBuffer->Release();
//	if (m_IndexBuffer)
//		m_IndexBuffer->Release();
//
//	for (INT i = 0; i < mSubMeshes.size(); i++)
//		delete mSubMeshes[i];
//	mSubMeshes.clear();
//
//}
//
//
//BOOL VBOMesh::Initialize(const FbxMesh* pMesh)
//{
//	if (!pMesh->GetNode())
//		return false;
//
//	const INT lPolygonCount = pMesh->GetPolygonCount();
//
//	// Count the polygon count of each material
//	FbxLayerElementArrayTemplate<INT>* lMaterialIndice = NULL;
//	FbxGeometryElement::EMappingMode lMaterialMappingMode = FbxGeometryElement::eNone;
//	if (pMesh->GetElementMaterial())
//	{
//		lMaterialIndice = &pMesh->GetElementMaterial()->GetIndexArray();
//		lMaterialMappingMode = pMesh->GetElementMaterial()->GetMappingMode();
//		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
//		{
//			FBX_ASSERT(lMaterialIndice->GetCount() == lPolygonCount);
//			if (lMaterialIndice->GetCount() == lPolygonCount)
//			{
//				// Count the faces of each material
//				for (INT lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
//				{
//					const INT lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
//					if (mSubMeshes.size() < lMaterialIndex + 1)
//					{
//						mSubMeshes.resize(lMaterialIndex + 1);
//					}
//					if (mSubMeshes[lMaterialIndex] == NULL)
//					{
//						mSubMeshes[lMaterialIndex] = new SubMesh;
//					}
//					mSubMeshes[lMaterialIndex]->TriangleCount += 1;
//				}
//
//				// Make sure we have no "holes" (NULL) in the mSubMeshes table. This can happen
//				// if, in the loop above, we resized the mSubMeshes by more than one slot.
//				for (INT i = 0; i < mSubMeshes.size(); i++)
//				{
//					if (mSubMeshes[i] == NULL)
//						mSubMeshes[i] = new SubMesh;
//				}
//
//				// Record the offset (how many vertex)
//				const INT lMaterialCount = mSubMeshes.size();
//				INT lOffset = 0;
//				for (INT lIndex = 0; lIndex < lMaterialCount; ++lIndex)
//				{
//					mSubMeshes[lIndex]->IndexOffset = lOffset;
//					lOffset += mSubMeshes[lIndex]->TriangleCount * 3;
//					// This will be used as counter in the following procedures, reset to zero
//					mSubMeshes[lIndex]->TriangleCount = 0;
//				}
//				FBX_ASSERT(lOffset == lPolygonCount * 3);
//			}
//		}
//	}
//
//	// All faces will use the same material.
//	if (mSubMeshes.size() == 0)
//	{
//		mSubMeshes.resize(1);
//		mSubMeshes[0] = new SubMesh();
//	}
//
//	// Congregate all the data of a mesh to be cached in VBOs.
//	// If normal or UV is by polygon vertex, record all vertex attributes by polygon vertex.
//	mHasNormal = pMesh->GetElementNormalCount() > 0;
//	mHasUV = pMesh->GetElementUVCount() > 0;
//	FbxGeometryElement::EMappingMode lNormalMappingMode = FbxGeometryElement::eNone;
//	FbxGeometryElement::EMappingMode lUVMappingMode = FbxGeometryElement::eNone;
//	if (mHasNormal)
//	{
//		lNormalMappingMode = pMesh->GetElementNormal(0)->GetMappingMode();
//		if (lNormalMappingMode == FbxGeometryElement::eNone)
//		{
//			mHasNormal = false;
//		}
//		if (mHasNormal && lNormalMappingMode != FbxGeometryElement::eByControlPoint)
//		{
//			mAllByControlPoint = false;
//		}
//	}
//	if (mHasUV)
//	{
//		lUVMappingMode = pMesh->GetElementUV(0)->GetMappingMode();
//		if (lUVMappingMode == FbxGeometryElement::eNone)
//		{
//			mHasUV = false;
//		}
//		if (mHasUV && lUVMappingMode != FbxGeometryElement::eByControlPoint)
//		{
//			mAllByControlPoint = false;
//		}
//	}
//
//	// Allocate the array memory, by control point or by polygon vertex.
//	INT lPolygonVertexCount = pMesh->GetControlPointsCount();
//	if (!mAllByControlPoint)
//	{
//		lPolygonVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;
//	}
//	FLOAT* lVertices = new FLOAT[lPolygonVertexCount * VERTEX_STRIDE];
//	UINT* lIndices = new UINT[lPolygonCount * TRIANGLE_VERTEX_COUNT];
//	FLOAT* lNormals = NULL;
//	if (mHasNormal)
//	{
//		lNormals = new FLOAT[lPolygonVertexCount * NORMAL_STRIDE];
//	}
//	FLOAT* lUVs = NULL;
//	FbxStringList lUVNames;
//	pMesh->GetUVSetNames(lUVNames);
//	const CHAR* lUVName = NULL;
//	if (mHasUV && lUVNames.GetCount())
//	{
//		lUVs = new FLOAT[lPolygonVertexCount * UV_STRIDE];
//		lUVName = lUVNames[0];
//	}
//
//	// Populate the array with vertex attribute, if by control point.
//	const FbxVector4* lControlPoints = pMesh->GetControlPoints();
//	FbxVector4 lCurrentVertex;
//	FbxVector4 lCurrentNormal;
//	FbxVector2 lCurrentUV;
//	if (mAllByControlPoint)
//	{
//		const FbxGeometryElementNormal* lNormalElement = NULL;
//		const FbxGeometryElementUV* lUVElement = NULL;
//		if (mHasNormal)
//		{
//			lNormalElement = pMesh->GetElementNormal(0);
//		}
//		if (mHasUV)
//		{
//			lUVElement = pMesh->GetElementUV(0);
//		}
//		for (INT lIndex = 0; lIndex < lPolygonVertexCount; ++lIndex)
//		{
//			// Save the vertex position.
//			lCurrentVertex = lControlPoints[lIndex];
//			lVertices[lIndex * VERTEX_STRIDE] = static_cast<FLOAT>(lCurrentVertex[0]);
//			lVertices[lIndex * VERTEX_STRIDE + 1] = static_cast<FLOAT>(lCurrentVertex[1]);
//			lVertices[lIndex * VERTEX_STRIDE + 2] = static_cast<FLOAT>(lCurrentVertex[2]);
//			lVertices[lIndex * VERTEX_STRIDE + 3] = 1;
//
//			// Save the normal.
//			if (mHasNormal)
//			{
//				INT lNormalIndex = lIndex;
//				if (lNormalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
//				{
//					lNormalIndex = lNormalElement->GetIndexArray().GetAt(lIndex);
//				}
//				lCurrentNormal = lNormalElement->GetDirectArray().GetAt(lNormalIndex);
//				lNormals[lIndex * NORMAL_STRIDE] = static_cast<FLOAT>(lCurrentNormal[0]);
//				lNormals[lIndex * NORMAL_STRIDE + 1] = static_cast<FLOAT>(lCurrentNormal[1]);
//				lNormals[lIndex * NORMAL_STRIDE + 2] = static_cast<FLOAT>(lCurrentNormal[2]);
//			}
//
//			// Save the UV.
//			if (mHasUV)
//			{
//				INT lUVIndex = lIndex;
//				if (lUVElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
//				{
//					lUVIndex = lUVElement->GetIndexArray().GetAt(lIndex);
//				}
//				lCurrentUV = lUVElement->GetDirectArray().GetAt(lUVIndex);
//				lUVs[lIndex * UV_STRIDE] = static_cast<FLOAT>(lCurrentUV[0]);
//				lUVs[lIndex * UV_STRIDE + 1] = static_cast<FLOAT>(lCurrentUV[1]);
//			}
//		}
//
//	}
//
//	INT lVertexCount = 0;
//	for (INT lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
//	{
//		// The material for current face.
//		INT lMaterialIndex = 0;
//		if (lMaterialIndice && lMaterialMappingMode == FbxGeometryElement::eByPolygon)
//		{
//			lMaterialIndex = lMaterialIndice->GetAt(lPolygonIndex);
//		}
//
//		// Where should I save the vertex attribute index, according to the material
//		const INT lIndexOffset = mSubMeshes[lMaterialIndex]->IndexOffset +
//			mSubMeshes[lMaterialIndex]->TriangleCount * 3;
//		for (INT lVerticeIndex = 0; lVerticeIndex < TRIANGLE_VERTEX_COUNT; ++lVerticeIndex)
//		{
//			const INT lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
//			// If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
//			// it is not guaranteed that the cache will work as expected.
//			if (lControlPointIndex >= 0)
//			{
//				if (mAllByControlPoint)
//				{
//					lIndices[lIndexOffset + lVerticeIndex] = static_cast<UINT>(lControlPointIndex);
//				}
//				// Populate the array with vertex attribute, if by polygon vertex.
//				else
//				{
//					lIndices[lIndexOffset + lVerticeIndex] = static_cast<UINT>(lVertexCount);
//
//					lCurrentVertex = lControlPoints[lControlPointIndex];
//					lVertices[lVertexCount * VERTEX_STRIDE] = static_cast<FLOAT>(lCurrentVertex[0]);
//					lVertices[lVertexCount * VERTEX_STRIDE + 1] = static_cast<FLOAT>(lCurrentVertex[1]);
//					lVertices[lVertexCount * VERTEX_STRIDE + 2] = static_cast<FLOAT>(lCurrentVertex[2]);
//					lVertices[lVertexCount * VERTEX_STRIDE + 3] = 1;
//
//					if (mHasNormal)
//					{
//						pMesh->GetPolygonVertexNormal(lPolygonIndex, lVerticeIndex, lCurrentNormal);
//						lNormals[lVertexCount * NORMAL_STRIDE] = static_cast<FLOAT>(lCurrentNormal[0]);
//						lNormals[lVertexCount * NORMAL_STRIDE + 1] = static_cast<FLOAT>(lCurrentNormal[1]);
//						lNormals[lVertexCount * NORMAL_STRIDE + 2] = static_cast<FLOAT>(lCurrentNormal[2]);
//					}
//
//					if (mHasUV)
//					{
//						bool lUnmappedUV;
//						pMesh->GetPolygonVertexUV(lPolygonIndex, lVerticeIndex, lUVName, lCurrentUV, lUnmappedUV);
//						lUVs[lVertexCount * UV_STRIDE] = static_cast<FLOAT>(lCurrentUV[0]);
//						lUVs[lVertexCount * UV_STRIDE + 1] = static_cast<FLOAT>(lCurrentUV[1]);
//					}
//				}
//			}
//			++lVertexCount;
//		}
//		mSubMeshes[lMaterialIndex]->TriangleCount += 1;
//	}
//
//
//	if (m_Vertex.size() != 0)
//		m_Vertex.clear();
//	m_Vertex.resize(lPolygonVertexCount);
//
//	for (INT i = 0; i < lPolygonVertexCount; ++i)
//	{
//		m_Vertex[i].Position.x = lVertices[i * VERTEX_STRIDE];
//		m_Vertex[i].Position.y = lVertices[i * VERTEX_STRIDE + 1];
//		m_Vertex[i].Position.z = lVertices[i * VERTEX_STRIDE + 2];
//
//		if (mHasNormal)
//		{
//			m_Vertex[i].Normal.x = lNormals[i * NORMAL_STRIDE];
//			m_Vertex[i].Normal.y = lNormals[i * NORMAL_STRIDE + 1];
//			m_Vertex[i].Normal.z = lNormals[i * NORMAL_STRIDE + 2];
//		}
//		if (mHasUV)
//		{
//			m_Vertex[i].TexCoord.x = lUVs[i * UV_STRIDE];
//			m_Vertex[i].TexCoord.y = lUVs[i * UV_STRIDE + 1];
//		}
//	}
//
//	if (m_Index.size() != 0)
//		m_Index.clear();
//	m_Index.resize(lPolygonCount * TRIANGLE_VERTEX_COUNT);
//	for (INT i = 0; i < lPolygonCount * TRIANGLE_VERTEX_COUNT; ++i)
//	{
//		m_Index[i] = lIndices[i];
//	}
//
//	D3D11_BUFFER_DESC bd;
//	ZeroMemory(&bd, sizeof(bd));
//	bd.Usage = D3D11_USAGE_DEFAULT;
//	bd.ByteWidth = sizeof(VERTEX_3D) * m_Vertex.size();
//	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//	bd.CPUAccessFlags = 0;
//
//	D3D11_SUBRESOURCE_DATA sd;
//	ZeroMemory(&sd, sizeof(sd));
//	sd.pSysMem = m_Vertex.data();
//
//	CRenderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);
//
//	D3D11_BUFFER_DESC bd;
//	ZeroMemory(&bd, sizeof(bd));
//	bd.Usage = D3D11_USAGE_DEFAULT;
//	bd.ByteWidth = sizeof(UINT) * m_Index.size();
//	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
//	bd.CPUAccessFlags = 0;
//
//	D3D11_SUBRESOURCE_DATA sd;
//	ZeroMemory(&sd, sizeof(sd));
//	sd.pSysMem = m_Index.data();
//
//	CRenderer::GetDevice()->CreateBuffer(&bd, &sd, &m_IndexBuffer);
//
//	delete[] lIndices;
//	delete[] lVertices;
//	if (mHasNormal)
//		delete[] lNormals;
//	if (mHasUV)
//		delete[] lUVs;
//
//	return true;
//}
//
//
//void VBOMesh::UpdateVertexPosition(const FbxMesh* pMesh, const FbxVector4* pVertices)
//{
//	// Convert to the same sequence with data in GPU.
//	FLOAT* lVertices = NULL;
//	INT lVertexCount = 0;
//	if (mAllByControlPoint)
//	{
//		lVertexCount = pMesh->GetControlPointsCount();
//		lVertices = new FLOAT[lVertexCount * VERTEX_STRIDE];
//		for (INT lIndex = 0; lIndex < lVertexCount; ++lIndex)
//		{
//			lVertices[lIndex * VERTEX_STRIDE] = static_cast<FLOAT>(pVertices[lIndex][0]);
//			lVertices[lIndex * VERTEX_STRIDE + 1] = static_cast<FLOAT>(pVertices[lIndex][1]);
//			lVertices[lIndex * VERTEX_STRIDE + 2] = static_cast<FLOAT>(pVertices[lIndex][2]);
//			lVertices[lIndex * VERTEX_STRIDE + 3] = 1;
//		}
//	}
//	else
//	{
//		const INT lPolygonCount = pMesh->GetPolygonCount();
//		lVertexCount = lPolygonCount * TRIANGLE_VERTEX_COUNT;
//		lVertices = new FLOAT[lVertexCount * VERTEX_STRIDE];
//
//		lVertexCount = 0;
//		for (INT lPolygonIndex = 0; lPolygonIndex < lPolygonCount; ++lPolygonIndex)
//		{
//			for (INT lVerticeIndex = 0; lVerticeIndex < TRIANGLE_VERTEX_COUNT; ++lVerticeIndex)
//			{
//				const INT lControlPointIndex = pMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
//				// If the lControlPointIndex is -1, we probably have a corrupted mesh data. At this point,
//				// it is not guaranteed that the cache will work as expected.
//				if (lControlPointIndex >= 0)
//				{
//					lVertices[lVertexCount * VERTEX_STRIDE] = static_cast<FLOAT>(pVertices[lControlPointIndex][0]);
//					lVertices[lVertexCount * VERTEX_STRIDE + 1] = static_cast<FLOAT>(pVertices[lControlPointIndex][1]);
//					lVertices[lVertexCount * VERTEX_STRIDE + 2] = static_cast<FLOAT>(pVertices[lControlPointIndex][2]);
//					lVertices[lVertexCount * VERTEX_STRIDE + 3] = 1;
//				}
//				++lVertexCount;
//			}
//		}
//	}
//
//
//	// Transfer into GPU.
//	if (lVertices)
//	{
//		for (INT i = 0; i < (pMesh->GetPolygonCount() * TRIANGLE_VERTEX_COUNT); ++i)
//		{
//			m_Vertex[i].Position.x = lVertices[i * VERTEX_STRIDE];
//			m_Vertex[i].Position.x = lVertices[i * VERTEX_STRIDE + 1];
//			m_Vertex[i].Position.x = lVertices[i * VERTEX_STRIDE + 2];
//		}
//
//		D3D11_BUFFER_DESC bd;
//		ZeroMemory(&bd, sizeof(bd));
//		bd.Usage = D3D11_USAGE_DEFAULT;
//		bd.ByteWidth = sizeof(VERTEX_3D) * m_Vertex.size();
//		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
//		bd.CPUAccessFlags = 0;
//
//		D3D11_SUBRESOURCE_DATA sd;
//		ZeroMemory(&sd, sizeof(sd));
//		sd.pSysMem = m_Vertex.data();
//
//		if (m_VertexBuffer)
//			m_VertexBuffer->Release();
//		CRenderer::GetDevice()->CreateBuffer(&bd, &sd, &m_VertexBuffer);
//
//		delete[] lVertices;
//	}
//}
//
//
//void VBOMesh::Draw(INT pMaterialIndex) const
//{
//
//	// Where to start.
//	//INT lOffset = mSubMeshes[pMaterialIndex]->IndexOffset * sizeof(UINT);
//
//	//const INT lElementCount = mSubMeshes[pMaterialIndex]->TriangleCount * 3;
//
//}
//
//
//MaterialCache::MaterialCache()
//{
//	m_Material.Shinness = 0;
//}
//
//
//MaterialCache::~MaterialCache()
//{
//
//}
//
//
//// Bake material properties.
//BOOL MaterialCache::Initialize(const FbxSurfaceMaterial* pMaterial)
//{
//	const FbxDouble3 lEmissive = GetMaterialProperty(pMaterial,
//		FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, m_MaterialTexturePath.Emissive);
//	m_Material.Emissive.r = static_cast<FLOAT>(lEmissive[0]);
//	m_Material.Emissive.g = static_cast<FLOAT>(lEmissive[1]);
//	m_Material.Emissive.b = static_cast<FLOAT>(lEmissive[2]);
//	m_Material.Emissive.a = 1.0f;
//
//	const FbxDouble3 lAmbient = GetMaterialProperty(pMaterial,
//		FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, m_MaterialTexturePath.Ambient);
//	m_Material.Ambient.r = static_cast<FLOAT>(lAmbient[0]);
//	m_Material.Ambient.g = static_cast<FLOAT>(lAmbient[1]);
//	m_Material.Ambient.b = static_cast<FLOAT>(lAmbient[2]);
//	m_Material.Ambient.a = 1.0f;
//
//	const FbxDouble3 lDiffuse = GetMaterialProperty(pMaterial,
//		FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, m_MaterialTexturePath.Diffuse);
//	m_Material.Diffuse.r = static_cast<FLOAT>(lDiffuse[0]);
//	m_Material.Diffuse.g = static_cast<FLOAT>(lDiffuse[1]);
//	m_Material.Diffuse.b = static_cast<FLOAT>(lDiffuse[2]);
//	m_Material.Diffuse.a = 1.0f;
//
//	const FbxDouble3 lSpecular = GetMaterialProperty(pMaterial,
//		FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, m_MaterialTexturePath.Specular);
//	m_Material.Specular.r = static_cast<FLOAT>(lSpecular[0]);
//	m_Material.Specular.g = static_cast<FLOAT>(lSpecular[1]);
//	m_Material.Specular.b = static_cast<FLOAT>(lSpecular[2]);
//	m_Material.Specular.a = 1.0f;
//
//	FbxProperty lShininessProperty = pMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
//	if (lShininessProperty.IsValid())
//	{
//		DOUBLE lShininess = lShininessProperty.Get<FbxDouble>();
//		m_Material.Shinness = static_cast<FLOAT>(lShininess);
//	}
//
//	return true;
//}
//
//
//MATERIAL MaterialCache::GetDefaultMaterial()
//{
//	MATERIAL lMaterial;
//	lMaterial.Emission = COLOR(0.0f, 0.0f, 0.0f, 1.0f);
//	lMaterial.Ambient = COLOR(0.0f, 0.0f, 0.0f, 1.0f);
//	lMaterial.Diffuse = COLOR(0.0f, 0.0f, 0.0f, 1.0f);
//	lMaterial.Specular = COLOR(0.0f, 0.0f, 0.0f, 1.0f);
//	lMaterial.Shininess = 0;
//	return lMaterial;
//}
//
//
//
//CReadFBX::CReadFBX()
//{
//
//	m_Manager = NULL;
//	m_Scene = NULL;
//	m_Importer = NULL;
//
//	m_CurrentAnimLayer = NULL;
//
//	m_PoseIndex = -1;
//
//	InitializeSdkObjects();
//
//}
//
//
//CReadFBX::~CReadFBX()
//{
//
//	DestroySdkObjects();
//
//}
//
//
//void CReadFBX::InitializeSdkObjects()
//{
//
//	//The first thing to do is to create the FBX Manager which is the object allocator for almost all the classes in the SDK
//	m_Manager = FbxManager::Create();
//	if (!m_Manager)
//	{
//		MessageBox(GetWindow(), "Error: Unable to create FBX Manager!", "FbxManagerError", MB_OK);
//		exit(1);
//	}
//	
//	//Create an IOSettings object. This object holds all import/export settings.
//	FbxIOSettings* ios = FbxIOSettings::Create(m_Manager, IOSROOT);
//	m_Manager->SetIOSettings(ios);
//
//	//Load plugins from the executable directory (optional)
//	FbxString lPath = FbxGetApplicationDirectory();
//	m_Manager->LoadPluginsDirectory(lPath.Buffer());
//
//	//Create an FBX scene. This object holds most objects imported/exported from/to files.
//	m_Scene = FbxScene::Create(m_Manager, "My FBX read Scene");
//	if (!m_Scene)
//	{
//		MessageBox(GetWindow(), "Error: Unable to create FBX scene!", "FbxSceneError", MB_OK);
//		exit(1);
//	}
//
//}
//
//
//void CReadFBX::DestroySdkObjects()
//{
//
//	if (m_Manager)
//		m_Manager->Destroy();
//
//}
//
//
//void CReadFBX::InitializeImporter(string path)
//{
//
//	if (m_Manager)
//	{
//		// Create the importer.
//		INT lFileFormat = -1;
//		m_Importer = FbxImporter::Create(m_Manager, "");
//		if (!m_Manager->GetIOPluginRegistry()->DetectReaderFileFormat(path.c_str(), lFileFormat))
//		{
//			// Unrecognizable file format. Try to fall back to FbxImporter::eFBX_BINARY
//			lFileFormat = m_Manager->GetIOPluginRegistry()->FindReaderIDByDescription("FBX binary (*.fbx)");
//		}
//
//		// Initialize the importer by providing a filename.
//		if (m_Importer->Initialize(path.c_str(), lFileFormat) != true)
//		{
//			MessageBox(GetWindow(), "Error: Unable to Importer FBX!", "FbxImporterError", MB_OK);
//			exit(1);
//		}
//	}
//
//}
//
//
//BOOL CReadFBX::LoadFile(string path)
//{
//
//	BOOL lResult = false;
//
//	if (m_Importer->Import(m_Scene) == true)
//	{
//
//		// Check the scene integrity!
//		FbxStatus status;
//		FbxArray<FbxString*> details;
//		FbxSceneCheckUtility sceneCheck(FbxCast<FbxScene>(m_Scene), &status, &details);
//		lResult = sceneCheck.Validate(FbxSceneCheckUtility::eCkeckData);
//
//		if (lResult == false)
//		{
//
//			if (details.GetCount())
//			{
//				MessageBox(GetWindow(), "Scene integrity verification failed with the following errors", "FbxImporterError", MB_OK);
//
//				for (INT i = 0; i < details.GetCount(); i++)
//					MessageBox(GetWindow(), details[i]->Buffer(), "ImporterDetails", MB_OK);
//
//				FbxArrayDelete<FbxString*>(details);
//			}
//
//		}
//
//		if (lResult)
//		{
//
//			// Convert Axis System to what is used in this example, if needed
//			FbxAxisSystem SceneAxisSystem = m_Scene->GetGlobalSettings().GetAxisSystem();
//			FbxAxisSystem OurAxisSystem(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eLeftHanded);
//			if (SceneAxisSystem != OurAxisSystem)
//			{
//				OurAxisSystem.ConvertScene(m_Scene);
//			}
//
//			// Convert Unit System to what is used in this example, if needed
//			FbxSystemUnit SceneSystemUnit = m_Scene->GetGlobalSettings().GetSystemUnit();
//			if (SceneSystemUnit.GetScaleFactor() != 1.0)
//			{
//				//The unit in this example is centimeter.
//				FbxSystemUnit::cm.ConvertScene(m_Scene);
//			}
//
//			// Get the list of all the animation stack.
//			m_Scene->FillAnimStackNameArray(m_AnimStackNameArray);
//
//			// Convert mesh, NURBS and patch into triangle mesh
//			FbxGeometryConverter lGeomConverter(m_Manager);
//			lGeomConverter.Triangulate(m_Scene, /*replace*/true);
//
//			// Convert any .PC2 point cache data into the .MC format for
//			// vertex cache deformer playback.
//			PreparePointCacheData(m_Scene, m_Cache_Start, m_Cache_Stop);
//
//			// Get the list of pose in the scene
//			FillPoseArray(m_Scene, m_PoseArray);
//
//			// Initialize the frame period.
//			m_FrameTime.SetTime(0, 0, 0, 1, 0, m_Scene->GetGlobalSettings().GetTimeMode());
//
//		}
//
//	}
//	else
//	{
//
//		MessageBox(GetWindow(), m_Importer->GetStatus().GetErrorString(), "FbxImporterError", MB_OK);
//
//	}
//
//	m_Importer->Destroy();
//	m_Importer = NULL;
//
//	return lResult;
//
//}
//
//
//void CReadFBX::PreparePointCacheData(FbxScene* pScene, FbxTime& pCache_Start, FbxTime& pCache_Stop)
//{
//
//	// This function show how to cycle through scene elements in a linear way.
//	const INT lNodeCount = pScene->GetSrcObjectCount<FbxNode>();
//	FbxStatus lStatus;
//
//	for (INT lIndex = 0; lIndex < lNodeCount; lIndex++)
//	{
//		FbxNode* lNode = pScene->GetSrcObject<FbxNode>(lIndex);
//
//		if (lNode->GetGeometry())
//		{
//			INT i, lVertexCacheDeformerCount = lNode->GetGeometry()->GetDeformerCount(FbxDeformer::eVertexCache);
//
//			// There should be a maximum of 1 Vertex Cache Deformer for the moment
//			lVertexCacheDeformerCount = lVertexCacheDeformerCount > 0 ? 1 : 0;
//
//			for (i = 0; i < lVertexCacheDeformerCount; ++i)
//			{
//				// Get the Point Cache object
//				FbxVertexCacheDeformer* lDeformer = static_cast<FbxVertexCacheDeformer*>(lNode->GetGeometry()->GetDeformer(i, FbxDeformer::eVertexCache));
//				if (!lDeformer) continue;
//				FbxCache* lCache = lDeformer->GetCache();
//				if (!lCache) continue;
//
//				// Process the point cache data only if the constraint is active
//				if (lDeformer->Active.Get())
//				{
//					if (lCache->GetCacheFileFormat() == FbxCache::eMaxPointCacheV2)
//					{
//						// This code show how to convert from PC2 to MC point cache format
//						// turn it on if you need it.
//#if 0 
//						if (!lCache->ConvertFromPC2ToMC(FbxCache::eMCOneFile,
//							FbxTime::GetFrameRate(pScene->GetGlobalTimeSettings().GetTimeMode())))
//						{
//							// Conversion failed, retrieve the error here
//							FbxString lTheErrorIs = lCache->GetStaus().GetErrorString();
//						}
//#endif
//					}
//					else if (lCache->GetCacheFileFormat() == FbxCache::eMayaCache)
//					{
//						// This code show how to convert from MC to PC2 point cache format
//						// turn it on if you need it.
//						//#if 0 
//						if (!lCache->ConvertFromMCToPC2(FbxTime::GetFrameRate(pScene->GetGlobalSettings().GetTimeMode()), 0, &lStatus))
//						{
//							// Conversion failed, retrieve the error here
//							FbxString lTheErrorIs = lStatus.GetErrorString();
//						}
//						//#endif
//					}
//
//
//					// Now open the cache file to read from it
//					if (!lCache->OpenFileForRead(&lStatus))
//					{
//						// Cannot open file 
//						FbxString lTheErrorIs = lStatus.GetErrorString();
//
//						// Set the deformer inactive so we don't play it back
//						lDeformer->Active = false;
//					}
//					else
//					{
//						// get the start and stop time of the cache
//						FbxTime lChannel_Start;
//						FbxTime lChannel_Stop;
//						INT lChannelIndex = lCache->GetChannelIndex(lDeformer->Channel.Get());
//						if (lCache->GetAnimationRange(lChannelIndex, lChannel_Start, lChannel_Stop))
//						{
//							// get the smallest start time
//							if (lChannel_Start < pCache_Start) pCache_Start = lChannel_Start;
//
//							// get the biggest stop time
//							if (lChannel_Stop > pCache_Stop)  pCache_Stop = lChannel_Stop;
//						}
//					}
//				}
//			}
//		}
//	}
//}
//
//
//void CReadFBX::FillPoseArray(FbxScene* pScene, FbxArray<FbxPose*>& pPoseArray)
//{
//
//	const INT lPoseCount = pScene->GetPoseCount();
//
//	for (INT i = 0; i < lPoseCount; ++i)
//	{
//		pPoseArray.Add(pScene->GetPose(i));
//	}
//
//}
//
//BOOL CReadFBX::SetCurrentAnimStack(INT pIndex)
//{
//
//	const INT lAnimStackCount = m_AnimStackNameArray.GetCount();
//	if (!lAnimStackCount || pIndex >= lAnimStackCount)
//	{
//		return false;
//	}
//
//	// select the base layer from the animation stack
//	FbxAnimStack* lCurrentAnimationStack = m_Scene->FindMember<FbxAnimStack>(m_AnimStackNameArray[pIndex]->Buffer());
//	if (lCurrentAnimationStack == NULL)
//	{
//		// this is a problem. The anim stack should be found in the scene!
//		return false;
//	}
//
//	// we assume that the first animation layer connected to the animation stack is the base layer
//	// (this is the assumption made in the FBXSDK)
//	m_CurrentAnimLayer = lCurrentAnimationStack->GetMember<FbxAnimLayer>();
//	m_Scene->SetCurrentAnimationStack(lCurrentAnimationStack);
//
//	FbxTakeInfo* lCurrentTakeInfo = m_Scene->GetTakeInfo(*(m_AnimStackNameArray[pIndex]));
//	if (lCurrentTakeInfo)
//	{
//		m_Start = lCurrentTakeInfo->mLocalTimeSpan.GetStart();
//		m_Stop = lCurrentTakeInfo->mLocalTimeSpan.GetStop();
//	}
//	else
//	{
//		// Take the time line value
//		FbxTimeSpan lTimeLineTimeSpan;
//		m_Scene->GetGlobalSettings().GetTimelineDefaultTimeSpan(lTimeLineTimeSpan);
//
//		m_Start = lTimeLineTimeSpan.GetStart();
//		m_Stop = lTimeLineTimeSpan.GetStop();
//	}
//
//	// check for smallest start with cache start
//	if (m_Cache_Start < m_Start)
//		m_Start = m_Cache_Start;
//
//	// check for biggest stop with cache stop
//	if (m_Cache_Stop > m_Stop)
//		m_Stop = m_Cache_Stop;
//
//	// move to beginning
//	m_CurrentTime = m_Start;
//
//	return true;
//
//}
//
//
//void CReadFBX::SetCurrentPoseIndex(INT pPoseIndex)
//{
//
//	m_PoseIndex = pPoseIndex;
//
//}
//
//
//FbxAMatrix CReadFBX::GetPoseMatrix(FbxPose* pPose, INT pNodeIndex)
//{
//
//	FbxAMatrix	lPoseMatrix;
//	FbxMatrix	lMatrix = pPose->GetMatrix(pNodeIndex);
//
//	memcpy((DOUBLE*)lPoseMatrix, (DOUBLE*)lMatrix, sizeof(lMatrix.mData));
//
//	return lPoseMatrix;
//
//}
//
//
//FbxAMatrix CReadFBX::GetGeometry(FbxNode* pNode)
//{
//
//	const FbxVector4 lT = pNode->GetGeometricTranslation(FbxNode::eSourcePivot);
//	const FbxVector4 lR = pNode->GetGeometricRotation(FbxNode::eSourcePivot);
//	const FbxVector4 lS = pNode->GetGeometricScaling(FbxNode::eSourcePivot);
//
//	return FbxAMatrix(lT, lR, lS);
//
//}
//
//
//FbxAMatrix CReadFBX::GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose, FbxAMatrix* pParentGlobalPosition)
//{
//
//	FbxAMatrix	lGlobalPosition;
//	BOOL		lPositionFound = false;
//
//	if (pPose)
//	{
//		int lNodeIndex = pPose->Find(pNode);
//
//		if (lNodeIndex > -1)
//		{
//			// The bind pose is always a global matrix.
//			// If we have a rest pose, we need to check if it is
//			// stored in global or local space.
//			if (pPose->IsBindPose() || !pPose->IsLocalMatrix(lNodeIndex))
//			{
//				lGlobalPosition = GetPoseMatrix(pPose, lNodeIndex);
//			}
//			else
//			{
//				// We have a local matrix, we need to convert it to
//				// a global space matrix.
//				FbxAMatrix lParentGlobalPosition;
//
//				if (pParentGlobalPosition)
//				{
//					lParentGlobalPosition = *pParentGlobalPosition;
//				}
//				else
//				{
//					if (pNode->GetParent())
//					{
//						lParentGlobalPosition = GetGlobalPosition(pNode->GetParent(), pTime, pPose);
//					}
//				}
//
//				FbxAMatrix lLocalPosition = GetPoseMatrix(pPose, lNodeIndex);
//				lGlobalPosition = lParentGlobalPosition * lLocalPosition;
//			}
//
//			lPositionFound = true;
//		}
//	}
//
//	if (!lPositionFound)
//	{
//		// There is no pose entry for that node, get the current global position instead.
//
//		// Ideally this would use parent global position and local position to compute the global position.
//		// Unfortunately the equation 
//		//    lGlobalPosition = pParentGlobalPosition * lLocalPosition
//		// does not hold when inheritance type is other than "Parent" (RSrs).
//		// To compute the parent rotation and scaling is tricky in the RrSs and Rrs cases.
//		lGlobalPosition = pNode->EvaluateGlobalTransform(pTime);
//	}
//
//	return lGlobalPosition;
//
//}
//
//
//void CReadFBX::ReadSkeleton(FbxNode* pNode, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition)
//{
//
//
//
//}
//
//
//void CReadFBX::ReadVertexCacheData(FbxMesh* pMesh, FbxTime& pTime, FbxVector4* pVertexArray)
//{
//
//
//
//}
//
//
//void CReadFBX::ReadMesh(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pGlobalPosition, FbxPose* pPose)
//{
//
//	FbxMesh*	lMesh = pNode->GetMesh();
//	const INT	lVertexCount = lMesh->GetControlPointsCount();
//
//	// No vertex to Read.
//	if (lVertexCount == 0)
//		return;
//
//	const VBOMesh* lMeshCache = static_cast<const VBOMesh*>(lMesh->GetUserDataPtr());
//
//	// If it has some defomer connection, update the vertices position
//	const BOOL lHasVertexCache = lMesh->GetDeformerCount(FbxDeformer::eVertexCache) &&
//		(static_cast<FbxVertexCacheDeformer*>(lMesh->GetDeformer(0, FbxDeformer::eVertexCache)))->Active.Get();
//	const BOOL lHasShape = lMesh->GetShapeCount() > 0;
//	const BOOL lHasSkin = lMesh->GetDeformerCount(FbxDeformer::eSkin) > 0;
//	const BOOL lHasDeformation = lHasVertexCache || lHasShape || lHasSkin;
//
//	FbxVector4* lVertexArray = NULL;
//	if (!(lMesh->GetUserDataPtr()) || lHasDeformation)
//	{
//
//		lVertexArray = new FbxVector4[lVertexCount];
//		memcpy(lVertexArray, lMesh->GetControlPoints(), lVertexCount * sizeof(FbxVector4));
//
//	}
//
//	if (lHasDeformation)
//	{
//
//		// Active vertex cache deformer will overwrite any other deformer
//		if (lHasVertexCache)
//		{
//
//			ReadVertexCacheData(lMesh, pTime, lVertexArray);
//
//		}
//		else
//		{
//
//			if (lHasShape)
//			{
//
//				// Deform the vertex array with the shapes.
//				ComputeShapeDeformation(lMesh, pTime, pAnimLayer, lVertexArray);
//
//			}
//
//			//we need to get the number of clusters
//			const int lSkinCount = lMesh->GetDeformerCount(FbxDeformer::eSkin);
//			int lClusterCount = 0;
//			for (int lSkinIndex = 0; lSkinIndex < lSkinCount; ++lSkinIndex)
//			{
//
//				lClusterCount += ((FbxSkin*)(lMesh->GetDeformer(lSkinIndex, FbxDeformer::eSkin)))->GetClusterCount();
//
//			}
//			if (lClusterCount)
//			{
//
//				// Deform the vertex array with the skin deformer.
//				ComputeSkinDeformation(pGlobalPosition, lMesh, pTime, lVertexArray, pPose);
//
//			}
//
//		}
//
//		if (lMeshCache)
//			lMeshCache->UpdateVertexPosition(lMesh, lVertexArray);
//
//	}
//
//	glPushMatrix();
//	glMultMatrixd((const double*)pGlobalPosition);
//
//	if (lMeshCache)
//	{
//		lMeshCache->BeginDraw(pShadingMode);
//		const int lSubMeshCount = lMeshCache->GetSubMeshCount();
//		for (int lIndex = 0; lIndex < lSubMeshCount; ++lIndex)
//		{
//			if (pShadingMode == SHADING_MODE_SHADED)
//			{
//				const FbxSurfaceMaterial* lMaterial = pNode->GetMaterial(lIndex);
//				if (lMaterial)
//				{
//					const MaterialCache* lMaterialCache = static_cast<const MaterialCache*>(lMaterial->GetUserDataPtr());
//					if (lMaterialCache)
//					{
//						lMaterialCache->SetCurrentMaterial();
//					}
//				}
//				else
//				{
//					// Draw green for faces without material
//					MaterialCache::SetDefaultMaterial();
//				}
//			}
//
//			lMeshCache->Draw(lIndex, pShadingMode);
//		}
//		lMeshCache->EndDraw();
//	}
//	else
//	{
//		// OpenGL driver is too lower and use Immediate Mode
//		glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
//		const int lPolygonCount = lMesh->GetPolygonCount();
//		for (int lPolygonIndex = 0; lPolygonIndex < lPolygonCount; lPolygonIndex++)
//		{
//			const int lVerticeCount = lMesh->GetPolygonSize(lPolygonIndex);
//			glBegin(GL_LINE_LOOP);
//			for (int lVerticeIndex = 0; lVerticeIndex < lVerticeCount; lVerticeIndex++)
//			{
//				int index = lMesh->GetPolygonVertex(lPolygonIndex, lVerticeIndex);
//				if (index < 0)
//				{
//					FBX_ASSERT_NOW("Invalid index!");
//					continue;
//				}
//				glVertex3dv((GLdouble*)lVertexArray[index]);
//			}
//			glEnd();
//		}
//	}
//
//	glPopMatrix();
//
//	delete[] lVertexArray;
//
//}
//
//
//void CReadFBX::ReadNode(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition, FbxPose* pPose)
//{
//
//	FbxNodeAttribute* lNodeAttribute = pNode->GetNodeAttribute();
//
//	if (lNodeAttribute)
//	{
//
//		if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eSkeleton)
//		{
//			ReadSkeleton(pNode, pParentGlobalPosition, pGlobalPosition);
//		}
//		// NURBS and patch have been converted into triangluation meshes.
//		else if (lNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
//		{
//			ReadMesh(pNode, pTime, pAnimLayer, pGlobalPosition, pPose);
//		}
//
//	}
//
//}
//
//
//void CReadFBX::ReadNodeRecursive(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer,
//	FbxAMatrix& pParentGlobalPosition, FbxPose* pPose)
//{
//
//	FbxAMatrix lGlobalPosition = GetGlobalPosition(pNode, pTime, pPose, &pParentGlobalPosition);
//
//	if (pNode->GetNodeAttribute())
//	{
//
//		// Geometry offset.
//		// it is not inherited by the children.
//		FbxAMatrix lGeometryOffset = GetGeometry(pNode);
//		FbxAMatrix lGlobalOffPosition = lGlobalPosition * lGeometryOffset;
//
//		ReadNode(pNode, pTime, pAnimLayer, pParentGlobalPosition, lGlobalOffPosition, pPose);
//
//	}
//
//	const INT lChildCount = pNode->GetChildCount();
//	for (INT lChildIndex = 0; lChildIndex < lChildCount; ++lChildIndex)
//	{
//
//		ReadNodeRecursive(pNode->GetChild(lChildIndex), pTime, pAnimLayer, lGlobalPosition, pPose);
//
//	}
//
//}
//
//
//void CReadFBX::Init(string path)
//{
//
//	InitializeImporter(path);
//
//	if (!LoadFile(path))
//		MessageBox(GetWindow(), "LoadFile error", "Error", MB_OK);
//
//	SetCurrentPoseIndex(0);
//	SetCurrentAnimStack(0);
//
//	FbxPose* lPose = NULL;
//	if (m_PoseIndex != -1)
//		lPose = m_Scene->GetPose(m_PoseIndex);
//
//	FbxAMatrix lDummyGlobalPosition;
//	ReadNodeRecursive(m_Scene->GetRootNode(), m_CurrentTime, m_CurrentAnimLayer, lDummyGlobalPosition, lPose);
//
//
//
//}