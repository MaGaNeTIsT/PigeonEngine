#pragma once
//
//
//
//#include "../../../../../Entry/EngineMain.h"
//#include "../../Headers/Base/CRenderDevice.h"
//
//
//
//
//
//// Property cache, value and animation curve.
//struct PropertyChannel
//{
//    PropertyChannel() : mAnimCurve(NULL), mValue(0.0f) {}
//    // Query the channel value at specific time.
//    FLOAT Get(const FbxTime& pTime) const
//    {
//        if (mAnimCurve)
//        {
//            return mAnimCurve->Evaluate(pTime);
//        }
//        else
//        {
//            return mValue;
//        }
//    }
//
//    FbxAnimCurve* mAnimCurve;
//    FLOAT mValue;
//};
//
//
//
//// Save mesh vertices, normals, UVs and indices in GPU with OpenGL Vertex Buffer Objects
//class VBOMesh
//{
//public:
//
//    VBOMesh();
//    ~VBOMesh();
//
//    // Save up data into GPU buffers.
//    BOOL Initialize(const FbxMesh* pMesh);
//
//    // Update vertex positions for deformed meshes.
//    void UpdateVertexPosition(const FbxMesh* pMesh, const FbxVector4* pVertices);
//
//    // Draw all the faces with specific material with given shading mode.
//    void Draw(INT pMaterialIndex) const;
//
//    // Get the count of material groups
//    INT GetSubMeshCount() const { return mSubMeshes.size(); }
//
//private:
//
//    // For every material, record the offsets in every VBO and triangle counts
//    struct SubMesh
//    {
//        SubMesh() : IndexOffset(0), TriangleCount(0) {}
//
//        INT IndexOffset;
//        INT TriangleCount;
//    };
//
//
//    vector<SubMesh*>    mSubMeshes;
//    BOOL                mHasNormal;
//    BOOL                mHasUV;
//    BOOL                mAllByControlPoint; // Save data in VBO by control point or by polygon vertex.
//
//    vector<CustomStruct::CVertex3DData>   m_Vertex;
//    vector<UINT>        m_Index;
//
//    ID3D11Buffer*       m_VertexBuffer;
//    ID3D11Buffer*       m_IndexBuffer;
//
//};
//
//
//
//struct FBXMaterial
//{
//    COLOR   Emissive;
//    COLOR   Ambient;
//    COLOR   Diffuse;
//    COLOR   Specular;
//    FLOAT   Shinness;
//};
//
//
//struct FBXMaterialTexturePath
//{
//    vector<string>   Emissive;
//    vector<string>   Ambient;
//    vector<string>   Diffuse;
//    vector<string>   Specular;
//};
//
//
//// Cache for FBX material
//class MaterialCache
//{
//public:
//
//    MaterialCache();
//    ~MaterialCache();
//
//    BOOL Initialize(const FbxSurfaceMaterial* pMaterial);
//
//    // Set material colors and binding diffuse texture if exists.
//    FBXMaterial GetCurrentMaterial() const { return m_Material; }
//    FBXMaterialTexturePath GetCurrentMaterialTexturePath() const { return m_MaterialTexturePath; }
//
//    BOOL HasTexture() const { return m_MaterialTexturePath.Diffuse.size() != 0; }
//
//    // Set default green color.
//    static MATERIAL GetDefaultMaterial();
//
//private:
//
//    FBXMaterial             m_Material;
//    FBXMaterialTexturePath  m_MaterialTexturePath;
//
//};
//
//
//
//class CReadFBX
//{
//
//private:
//
//	FbxManager*							m_Manager;
//	FbxScene*							m_Scene;
//	FbxImporter*						m_Importer;
//
//	FbxAnimLayer*						m_CurrentAnimLayer;
//
//	INT									m_PoseIndex;
//	FbxArray<FbxString*>				m_AnimStackNameArray;
//	FbxArray<FbxPose*>					m_PoseArray;
//
//	mutable FbxTime						m_FrameTime, m_Start, m_Stop, m_CurrentTime;
//	mutable FbxTime						m_Cache_Start, m_Cache_Stop;
//
//	BOOL								m_SupportVBO;
//
//private:
//
//	void								InitializeSdkObjects();
//	void								DestroySdkObjects();
//	void								InitializeImporter(string path);
//	BOOL								LoadFile(string path);
//
//	void								PreparePointCacheData(FbxScene* pScene, FbxTime& pCache_Start, FbxTime& pCache_Stop);
//	void								FillPoseArray(FbxScene* pScene, FbxArray<FbxPose*>& pPoseArray);
//
//	BOOL								SetCurrentAnimStack(INT pIndex);
//	void								SetCurrentPoseIndex(INT pPoseIndex);
//
//	FbxAMatrix							GetPoseMatrix(FbxPose* pPose, INT pNodeIndex);
//	FbxAMatrix							GetGeometry(FbxNode* pNode);
//	FbxAMatrix							GetGlobalPosition(FbxNode* pNode, const FbxTime& pTime, FbxPose* pPose = (FbxPose*)0, FbxAMatrix* pParentGlobalPosition = (FbxAMatrix*)0);
//
//	void								ReadSkeleton(FbxNode* pNode, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition);
//    
//    void                                ReadVertexCacheData(FbxMesh* pMesh, FbxTime& pTime, FbxVector4* pVertexArray);
//    void								ReadMesh(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pGlobalPosition, FbxPose* pPose);
//	void								ReadNode(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pParentGlobalPosition, FbxAMatrix& pGlobalPosition, FbxPose* pPose);
//	void                                ReadNodeRecursive(FbxNode* pNode, FbxTime& pTime, FbxAnimLayer* pAnimLayer, FbxAMatrix& pParentGlobalPosition, FbxPose* pPose);
//
//public:
//
//	CReadFBX();
//	~CReadFBX();
//
//public:
//
//	void								Init(string path);
//
//
//};