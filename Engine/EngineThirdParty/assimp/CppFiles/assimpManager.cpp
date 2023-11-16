#include "../Headers/assimpManager.h"
#include <Base/DataStructure/Container/Map.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace PigeonEngine
{
#define CUSTOM_IS_QNAN(f)	(f != f)

#if _EDITOR_ONLY

	static void RegisterClassTypes()
	{
		RegisterClassType<CAssimpManager, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

#endif

	struct StoredMeshLayoutDesc
	{
		EVertexLayoutType	BaseVertexLayout;
		UINT32				TryStoredLayoutNum;
		UINT32				TryStoredLayoutSlot[EMesh::MeshVertexLayoutPartMaxNum];
	};

	static Assimp::Importer* _GAssetImporter = nullptr;

	PE_INLINE static Quaternion AssimpTranslateQuaternion(const aiQuaternion& v)
	{
		return (Quaternion(v.x, v.y, v.z, v.w));
	}
	PE_INLINE static Vector3 AssimpTranslateVector3(const aiVector3D& v)
	{
		return (Vector3(v.x, v.y, v.z));
	}
	PE_INLINE static void AssimpTranslateMatrix(const aiMatrix4x4& m, Vector3& pos, Quaternion& rot, Vector3& scl)
	{
		aiVector3D aiScl, aiPos; aiQuaternion aiRot;
		m.Decompose(aiScl, aiRot, aiPos);
		pos = AssimpTranslateVector3(aiPos);
		scl = AssimpTranslateVector3(aiScl);
		rot = AssimpTranslateQuaternion(aiRot);
	}
	PE_INLINE static Matrix4x4 AssimpTranslateMatrix(const aiMatrix4x4& m, const BOOL32& useOrigin = TRUE)
	{
		if (useOrigin)
		{
			return (Matrix4x4(DirectX::XMMATRIX(&(m.a1))));
		}
		else
		{
			aiVector3D scl, pos; aiQuaternion rot;
			m.Decompose(scl, rot, pos);
			return (Matrix4x4(
				DirectX::XMMatrixTranspose(
					DirectX::XMMatrixAffineTransformation(
						DirectX::XMVectorSet(scl.x, scl.y, scl.z, 1.f),
						DirectX::XMVectorSet(0.f, 0.f, 0.f, 0.f),
						DirectX::XMVectorSet(rot.x, rot.y, rot.z, rot.w),
						DirectX::XMVectorSet(pos.x, pos.y, pos.z, 1.f)))));
		}
	}
	PE_INLINE static EString AssimpTranslateString(const aiString& s)
	{
		return (EString(s.C_Str()));
	}
	PE_INLINE static aiString AssimpTranslateString(const EString& s)
	{
		return (aiString(*s));
	}
	PE_INLINE static EAnimationBehaviourType AssimpTranslateAnimationBehaviour(aiAnimBehaviour v)
	{
		EAnimationBehaviourType Result = EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT;
		if (v == aiAnimBehaviour::aiAnimBehaviour_DEFAULT)
		{
			Result = EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT;
		}
		else if (v == aiAnimBehaviour::aiAnimBehaviour_CONSTANT)
		{
			Result = EAnimationBehaviourType::ANIMATION_BEHAVIOUR_CONSTANT;
		}
		else if (v == aiAnimBehaviour::aiAnimBehaviour_LINEAR)
		{
			Result = EAnimationBehaviourType::ANIMATION_BEHAVIOUR_LINEAR;
		}
		else if (v == aiAnimBehaviour::aiAnimBehaviour_REPEAT)
		{
			Result = EAnimationBehaviourType::ANIMATION_BEHAVIOUR_REPEAT;
		}
		return Result;
	}
	PE_INLINE static void StoreVertexData(const aiVector2D* InOriginDatas, const UINT32 InOriginDataNum, const UINT32 InStoredDataStrideIn32Bits, FLOAT*& OutStoredDatas)
	{
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored origin vertex data can not be null."), (!!InOriginDatas));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Number of stored origin vertex data is too small."), (InOriginDataNum >= 3u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored vertex data's stride must be bigger than 2 in 32bits."), (InStoredDataStrideIn32Bits >= 2u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored vertex data can not be null."), (!!OutStoredDatas));
		if (InOriginDatas && OutStoredDatas)
		{
			for (UINT32 i = 0u; i < InOriginDataNum; i++)
			{
				const aiVector2D& TempData = InOriginDatas[i];
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 0u] = TempData.x;
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 1u] = TempData.y;
			}
		}
	}
	PE_INLINE static void StoreVertexData(const aiVector3D* InOriginDatas, const UINT32 InOriginDataNum, const UINT32 InStoredDataStrideIn32Bits, FLOAT*& OutStoredDatas)
	{
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored origin vertex data can not be null."), (!!InOriginDatas));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Number of stored origin vertex data is too small."), (InOriginDataNum >= 3u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored vertex data's stride must be bigger than 2 in 32bits."), (InStoredDataStrideIn32Bits >= 2u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored vertex data can not be null."), (!!OutStoredDatas));
		if (InOriginDatas && OutStoredDatas)
		{
			for (UINT32 i = 0u; i < InOriginDataNum; i++)
			{
				const aiVector3D& TempData = InOriginDatas[i];
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 0u] = TempData.x;
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 1u] = TempData.y;
				if (InStoredDataStrideIn32Bits > 2u)
				{
					OutStoredDatas[i * InStoredDataStrideIn32Bits + 2u] = TempData.z;
				}
			}
		}
	}
	PE_INLINE static void StoreVertexData(const aiColor4D* InOriginDatas, const UINT32 InOriginDataNum, const UINT32 InStoredDataStrideIn32Bits, FLOAT*& OutStoredDatas)
	{
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored origin vertex data can not be null."), (!!InOriginDatas));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Number of stored origin vertex data is too small."), (InOriginDataNum >= 3u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored vertex data's stride must be bigger than 2 in 32bits."), (InStoredDataStrideIn32Bits >= 2u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored vertex data can not be null."), (!!OutStoredDatas));
		if (InOriginDatas && OutStoredDatas)
		{
			for (UINT32 i = 0u; i < InOriginDataNum; i++)
			{
				const aiColor4D& TempData = InOriginDatas[i];
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 0u] = TempData.r;
				OutStoredDatas[i * InStoredDataStrideIn32Bits + 1u] = TempData.g;
				if (InStoredDataStrideIn32Bits > 2u)
				{
					OutStoredDatas[i * InStoredDataStrideIn32Bits + 2u] = TempData.b;
					OutStoredDatas[i * InStoredDataStrideIn32Bits + 3u] = TempData.a;
				}
			}
		}
	}
	struct EReaderVertexWeights
	{
		EReaderVertexWeights()noexcept : MaxCount(0u)
		{
			for (UINT32 i = 0u; i < EEngineSettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM; i++)
			{
				Indices[i] = 0u;
				Weights[i] = 0.f;
			}
		}
		USHORT	MaxCount;
		USHORT	Indices[EEngineSettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM];
		FLOAT	Weights[EEngineSettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM];
	};
	PE_INLINE static void StoreSkinData(const TArray<EReaderVertexWeights>& InDatas, const UINT32 InMaxEffectNum, UINT32*& OutIndices, FLOAT*& OutWeights)
	{
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored origin skin data can not be null."), (InDatas.Length() > 0u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored skin data's effect bone num must be bigger than 1."), (InMaxEffectNum >= 1u));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored indices data can not be null."), (!!OutIndices));
		PE_CHECK((ENGINE_ASSET_ERROR), ("Stored weights data can not be null."), (!!OutWeights));
		if (OutIndices && OutWeights && InDatas.Length() > 0u)
		{
			for (UINT32 i = 0u, n = InDatas.Length(); i < n; i++)
			{
				const EReaderVertexWeights& TempData = InDatas[i];
				for (UINT32 EffectIndex = 0u; EffectIndex < InMaxEffectNum; EffectIndex++)
				{
					if (EffectIndex < TempData.MaxCount)
					{
						OutIndices[i * InMaxEffectNum + EffectIndex] = TempData.Indices[EffectIndex];
						OutWeights[i * InMaxEffectNum + EffectIndex] = TempData.Weights[EffectIndex];
					}
					else
					{
						OutIndices[i * InMaxEffectNum + EffectIndex] = 0u;
						OutWeights[i * InMaxEffectNum + EffectIndex] = 0.f;
					}
				}
			}
		}
	}
	static void AssimpGatherSingleNodeRecursion(const aiNode* InCurrentNode, TArray<const aiNode*>& OutNodes, TMap<const aiNode*, EString>& OutNodeNames, TMap<EString, SHORT>& OutNodeIndices)
	{
		if (InCurrentNode == nullptr)
		{
			return;
		}
		OutNodes.Add(InCurrentNode);
		{
			SHORT NodeIndex = static_cast<SHORT>(OutNodes.Length() - 1u);
			EString TempNodeName;
			if (InCurrentNode->mName.length > 0)
			{
				TempNodeName = AssimpTranslateString(InCurrentNode->mName);
			}
			else
			{
				TempNodeName = EEngineSettings::ENGINE_DEFAULT_NAME;
			}
			EString TempOldName = TempNodeName + "_";
			EString TempNewName = TempNodeName;
			UINT32 tempSameNameIndex = 0u;
			while (OutNodeIndices.ContainsKey(TempNewName))
			{
				TempNewName = TempOldName + ToString(tempSameNameIndex);
				tempSameNameIndex += 1u;
			}
			OutNodeIndices.Add(TempNewName, NodeIndex);
			OutNodeNames.Add(InCurrentNode, TempNewName);
		}
		for (UINT32 ChildIndex = 0u; ChildIndex < InCurrentNode->mNumChildren; ChildIndex++)
		{
			AssimpGatherSingleNodeRecursion(InCurrentNode->mChildren[ChildIndex], OutNodes, OutNodeNames, OutNodeIndices);
		}
	}
	static void AssimpReadNodeDefaultTransformRecursion(const aiNode* InCurrentNode, const TMap<const aiNode*, EString>& InNodeNames, const TMap<EString, SHORT>& InNodeIndices, TArray<EBoneData>& OutBoneDatas)
	{
		if (InCurrentNode == nullptr)
		{
			return;
		}
		{
			EString TempNodeName;
			if (InNodeNames.FindValue(InCurrentNode, TempNodeName))
			{
				SHORT TempNodeIndex = -2;
				if (InNodeIndices.FindValue(TempNodeName, TempNodeIndex))
				{
					if (TempNodeIndex >= 0)
					{
						AssimpTranslateMatrix(InCurrentNode->mTransformation,
							OutBoneDatas[TempNodeIndex].DefaultPosition,
							OutBoneDatas[TempNodeIndex].DefaultRotation,
							OutBoneDatas[TempNodeIndex].DefaultScaling);
					}
				}
			}
		}
		for (UINT32 indexChild = 0u; indexChild < InCurrentNode->mNumChildren; indexChild++)
		{
			AssimpReadNodeDefaultTransformRecursion(InCurrentNode->mChildren[indexChild], InNodeNames, InNodeIndices, OutBoneDatas);
		}
	}
	static void AssimpGatherSingleAnimationChannelData(const aiAnimation* InData, ESkeletonAnimationClip& OutData)
	{
		if (OutData.AnimationCurves.Length() > 0)
		{
			OutData.AnimationCurves.Clear();
		}
		for (UINT32 ChannelIndex = 0u, ChannelNum = InData->mNumChannels; ChannelIndex < ChannelNum; ChannelIndex++)
		{
			const aiNodeAnim* TempChannel = InData->mChannels[ChannelIndex];
			if ((!TempChannel) || (TempChannel->mNodeName.length < 1u))
			{
				continue;
			}
			OutData.AnimationCurves.Add(EBoneAnimationCurve(AssimpTranslateString(TempChannel->mNodeName)));
			EBoneAnimationCurve& NewCurve = OutData.AnimationCurves[OutData.AnimationCurves.Length() - 1u];
			if (TempChannel->mPositionKeys)
			{
				for (UINT32 PosKeyIndex = 0u; PosKeyIndex < TempChannel->mNumPositionKeys; PosKeyIndex++)
				{
					const aiVectorKey& TempChannelPosKey = TempChannel->mPositionKeys[PosKeyIndex];
					NewCurve.PositionKeys.Add(
						ETimeKey<Vector3, FLOAT>(
							static_cast<FLOAT>(TempChannelPosKey.mTime),
							AssimpTranslateVector3(TempChannelPosKey.mValue)));
				}
			}
			if (TempChannel->mRotationKeys)
			{
				for (UINT32 RotKeyIndex = 0u; RotKeyIndex < TempChannel->mNumRotationKeys; RotKeyIndex++)
				{
					const aiQuatKey& TempChannelRotKey = TempChannel->mRotationKeys[RotKeyIndex];
					NewCurve.RotationKeys.Add(
						ETimeKey<Quaternion, FLOAT>(
							static_cast<FLOAT>(TempChannelRotKey.mTime),
							AssimpTranslateQuaternion(TempChannelRotKey.mValue)));
				}
			}
			if (TempChannel->mScalingKeys)
			{
				for (UINT32 SclKeyIndex = 0u; SclKeyIndex < TempChannel->mNumScalingKeys; SclKeyIndex++)
				{
					const aiVectorKey& TempChannelSclKey = TempChannel->mScalingKeys[SclKeyIndex];
					NewCurve.ScalingKeys.Add(
						ETimeKey<Vector3, FLOAT>(
							static_cast<FLOAT>(TempChannelSclKey.mTime),
							AssimpTranslateVector3(TempChannelSclKey.mValue)));
				}
			}
			NewCurve.PreState = AssimpTranslateAnimationBehaviour(TempChannel->mPreState);
			NewCurve.PostState = AssimpTranslateAnimationBehaviour(TempChannel->mPostState);
			// TODO Is this necessary?
			{
				NewCurve.PositionKeys.Sort();
				NewCurve.RotationKeys.Sort();
				NewCurve.ScalingKeys.Sort();
			}
		}
	}
	CAssimpManager::CAssimpManager()
	{
#if _EDITOR_ONLY
		DebugName = ENGINE_ASSIMP_MANAGER_NAME;
#endif
	}
	CAssimpManager::~CAssimpManager()
	{
	}
	static BOOL32 FindMeshesAndVertexLayouts(const aiScene* InScene, TArray<const aiMesh*>& OutMeshes, TArray<TArray<RShaderSemanticType>>& OutLayouts, TArray<BOOL32>& OutIsSkeletalMesh)
	{
		if (!(InScene->HasMeshes()))
		{
			return FALSE;
		}
		if (OutLayouts.Length() > 0u)
		{
			OutLayouts.Clear();
		}
		if (OutMeshes.Length() > 0u)
		{
			OutMeshes.Clear();
		}
		if (OutIsSkeletalMesh.Length() > 0u)
		{
			OutIsSkeletalMesh.Clear();
		}
		for (UINT32 i = 0u, n = InScene->mNumMeshes; i < n; i++)
		{
			const aiMesh* PerMesh = InScene->mMeshes[i];
			if (PerMesh && PerMesh->HasPositions() && PerMesh->HasFaces())
			{
				OutMeshes.Add(PerMesh);
			}
		}
		if (OutMeshes.Length() < 1u)
		{
			return FALSE;
		}
		for (UINT32 i = 0u, n = OutMeshes.Length(); i < n; i++)
		{
			OutLayouts.Add(TArray<RShaderSemanticType>());
			OutIsSkeletalMesh.Add(FALSE);
			const aiMesh* PerMesh = OutMeshes[i];
			TArray<RShaderSemanticType>& PerMeshLayout = OutLayouts[OutLayouts.Length() - 1u];
			if (PerMesh->HasPositions())
			{
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_POSITION);
			}
			if (PerMesh->HasNormals())
			{
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_NORMAL);
			}
			if (PerMesh->GetNumUVChannels() > 0)
			{
				UINT32 CurrentUV = 0u;
				for (UINT32 IndexUV = 0u, MaxUV = AI_MAX_NUMBER_OF_TEXTURECOORDS; IndexUV < MaxUV; IndexUV++)
				{
					if (PerMesh->HasTextureCoords(IndexUV))
					{
						switch (CurrentUV)
						{
						case 0u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0);
							break;
						case 1u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD1);
							break;
						case 2u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD2);
							break;
						case 3u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD3);
							break;
						}
						CurrentUV++;
					}
				}
			}
			if (PerMesh->HasTangentsAndBitangents())
			{
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_TANGENT);
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_BINORMAL);
			}
			if (PerMesh->HasBones())
			{
				OutIsSkeletalMesh[OutIsSkeletalMesh.Length() - 1u] = TRUE;
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES);
				PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT);
			}
			if (PerMesh->GetNumColorChannels() > 0)
			{
				UINT32 CurrentColor = 0u;
				for (UINT32 IndexColor = 0u, MaxColor = AI_MAX_NUMBER_OF_COLOR_SETS; IndexColor < MaxColor; IndexColor++)
				{
					if (PerMesh->HasVertexColors(IndexColor))
					{
						switch (CurrentColor)
						{
						case 0u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR0);
							break;
						case 1u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR1);
							break;
						case 2u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR2);
							break;
						case 3u:
							PerMeshLayout.Add(RShaderSemanticType::SHADER_SEMANTIC_COLOR3);
							break;
						}
						CurrentColor++;
					}
				}
			}
		}
		return TRUE;
	}
	template<typename TDataType, typename TMeshType>
	static void TryAddMeshVertexPart(const TArray<const TDataType*>& InDatas, const TArray<UINT32>& InDataElementNum, const EVertexLayoutType InStoredLayoutBaseType, const UINT32* InStoredLayoutSlots, const UINT32 InStoredLayoutNum, const UINT32 InStrideIn32Bits, const UINT32 InSuccessAddMaxNum, TMeshType& OutMesh)
	{
		UINT32 AddElementCounter = 0u;
		for (UINT32 TryStoredSlotIndex = 0u; TryStoredSlotIndex < InStoredLayoutNum; TryStoredSlotIndex++)
		{
			if (AddElementCounter >= InDatas.Length())
			{
				return;
			}
			UINT32 TryStoredLayoutSlot = InStoredLayoutSlots[TryStoredSlotIndex];
			if (OutMesh.CheckVertexLayoutPartExist(InStoredLayoutBaseType, TryStoredLayoutSlot))
			{
				continue;
			}
			const UINT32 DataElementNum = InDataElementNum[AddElementCounter];
			EVertexData TempVertexData;
			TempVertexData.PartType = InStoredLayoutBaseType;
			TempVertexData.ElementNum = DataElementNum;
			TempVertexData.Stride = InStrideIn32Bits * sizeof(FLOAT);
			TempVertexData.Datas = new FLOAT[InStrideIn32Bits * DataElementNum];
			StoreVertexData(InDatas[AddElementCounter], DataElementNum, InStrideIn32Bits, TempVertexData.Datas);
			if (OutMesh.AddVertexElement(&TempVertexData))
			{
				AddElementCounter += 1u;
				if (AddElementCounter >= InSuccessAddMaxNum)
				{
					return;
				}
			}
			else
			{
				TempVertexData.Release();
			}
		}
	}
	static void TryAddMeshSkinPart(const TArray<const aiBone*>& InBones, const UINT32 InVertexNum, const EVertexLayoutType InStoredLayoutBaseType, const UINT32* InStoredLayoutSlots, const UINT32 InStoredLayoutNum, const UINT32 InSuccessAddMaxNum, ESkinnedMesh& OutMesh)
	{
		const UINT32 AssimpMeshSkinElementMaxCount = 1u;	//For assimp we assum that every mesh contains only ONE skinned data.
		{
			//Add bind pose for skinned datas.
			OutMesh.ClearBindPose();
			for (UINT32 i = 0u, n = InBones.Length(); i < n; i++)
			{
				const aiBone* TempBone = InBones[i];
				if (!TempBone)
				{
					continue;
				}
				OutMesh.AddBindPose(AssimpTranslateString(TempBone->mName), AssimpTranslateMatrix(TempBone->mOffsetMatrix, TRUE));
			}
			OutMesh.GenerateBindPoseIndex();
		}
		TArray<EReaderVertexWeights> TempVertexWeights;
		USHORT TempMaxWeightCount = 0u;
		{
			const ESkinnedMesh::EBindPoseIndex& TempBoneList = OutMesh.GetBindPoseIndex();
			if (TempBoneList.Length() == 0u)
			{
				return;
			}
			TempVertexWeights.Recapacity(InVertexNum);
			for (UINT32 i = 0u; i < InVertexNum; i++)
			{
				TempVertexWeights.Add(EReaderVertexWeights());
			}
			for (UINT32 i = 0u, n = InBones.Length(); i < n; i++)
			{
				const aiBone* TempBone = InBones[i];
				if (!TempBone)
				{
					continue;
				}
				for (UINT32 WeightIndex = 0u, WeightNum = TempBone->mNumWeights; WeightIndex < WeightNum; WeightIndex++)
				{
					const aiVertexWeight& TempAssimpWeight = TempBone->mWeights[WeightIndex];
					if (TempAssimpWeight.mVertexId < InVertexNum)
					{
						EReaderVertexWeights& ModifyWeight = TempVertexWeights[TempAssimpWeight.mVertexId];
						if (ModifyWeight.MaxCount < static_cast<USHORT>(EEngineSettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM))
						{
							if (TempBoneList.FindValue(AssimpTranslateString(TempBone->mName), ModifyWeight.Indices[ModifyWeight.MaxCount]))
							{
								ModifyWeight.Weights[ModifyWeight.MaxCount] = TempAssimpWeight.mWeight;
								ModifyWeight.MaxCount += 1u;
								TempMaxWeightCount = EMath::Max(TempMaxWeightCount, ModifyWeight.MaxCount);
							}
						}
					}
#if _EDITOR_ONLY
					else
					{
						PE_FAILED((ENGINE_ASSET_ERROR), ("Read skinned mesh bone's vertex id is not matching vertex max count, maybe used wrong bone list."));
					}
#endif
				}
			}
			if (TempMaxWeightCount == 0u)
			{
				return;
			}
		}
		if (TempMaxWeightCount == 3u)
		{
			TempMaxWeightCount = 4u;
		}
		OutMesh.SetEffectBoneNum(TempMaxWeightCount);
		{
			UINT32 AddElementCounter = 0u;
			for (UINT32 TryStoredSlotIndex = 0u; TryStoredSlotIndex < InStoredLayoutNum; TryStoredSlotIndex++)
			{
				if (AddElementCounter >= AssimpMeshSkinElementMaxCount)
				{
					return;
				}
				UINT32 TryStoredLayoutSlot = InStoredLayoutSlots[TryStoredSlotIndex];
				if (OutMesh.CheckVertexLayoutPartExist(InStoredLayoutBaseType, TryStoredLayoutSlot))
				{
					continue;
				}
				const UINT32 DataElementNum = InVertexNum;
				ESkinData TempSkinData;
				TempSkinData.PartType = InStoredLayoutBaseType;
				TempSkinData.ElementNum = DataElementNum;
				TempSkinData.Stride = TempMaxWeightCount * sizeof(FLOAT);
				TempSkinData.Indices = new UINT32[TempMaxWeightCount * DataElementNum];
				TempSkinData.Weights = new FLOAT[TempMaxWeightCount * DataElementNum];
				StoreSkinData(TempVertexWeights, TempMaxWeightCount, TempSkinData.Indices, TempSkinData.Weights);
				if (OutMesh.AddSkinElement(&TempSkinData))
				{
					AddElementCounter += 1u;
					if (AddElementCounter >= InSuccessAddMaxNum)
					{
						return;
					}
				}
				else
				{
					TempSkinData.Release();
				}
			}
		}
	}
	static TArray<StoredMeshLayoutDesc> GetShouldStoredMeshLayoutDescriptions(const RShaderSemanticType* InLayouts, const UINT32 InLayoutNum)
	{
		TArray<StoredMeshLayoutDesc> Result;
		TArray<UINT32> BlendIndicesSlots, BlendWeightsSlots;
		for (UINT32 LayoutIndex = 0u; LayoutIndex < InLayoutNum; LayoutIndex++)
		{
			RShaderSemanticType SemanticBaseType = GetShaderSemanticBaseType(InLayouts[LayoutIndex]);
			if (SemanticBaseType == RShaderSemanticType::SHADER_SEMANTIC_NONE)
			{
				continue;
			}
			//We calculate skin data after this part.
			if (SemanticBaseType == RShaderSemanticType::SHADER_SEMANTIC_BLENDINDICES)
			{
				BlendIndicesSlots.Add(GetShaderSemanticTypeSlot(InLayouts[LayoutIndex]));
				continue;
			}
			if (SemanticBaseType == RShaderSemanticType::SHADER_SEMANTIC_BLENDWEIGHT)
			{
				BlendWeightsSlots.Add(GetShaderSemanticTypeSlot(InLayouts[LayoutIndex]));
				continue;
			}
			UINT32 SemanticTypeSlot = GetShaderSemanticTypeSlot(InLayouts[LayoutIndex]);
			if (SemanticTypeSlot >= EMesh::MeshVertexLayoutPartMaxNum)
			{
				continue;
			}
			Result.Add(StoredMeshLayoutDesc());
			StoredMeshLayoutDesc& CurrentDesc = Result[Result.Length() - 1u];
			CurrentDesc.TryStoredLayoutNum += 1u;
			CurrentDesc.TryStoredLayoutSlot[CurrentDesc.TryStoredLayoutNum - 1u] = SemanticTypeSlot;

			//TODO We hard coding to finding VertexBaseType for now.
			CurrentDesc.BaseVertexLayout = TranslateSemanticBaseTypeToVertexBaseLayout(SemanticBaseType);
		}
#if !(_TEST_MODE || _SHIPPING_MODE)
		if ((BlendIndicesSlots.Length()) > 0u)
#else
		if (((BlendIndicesSlots.Length()) > 0u) && ((BlendIndicesSlots.Length()) == (BlendWeightsSlots.Length())))
#endif
		{
#if !(_TEST_MODE || _SHIPPING_MODE)
			PE_CHECK((ENGINE_ASSET_ERROR), ("number of [BlendIndices] and number of [BlendWeight] is not matched."), ((BlendIndicesSlots.Length()) == (BlendWeightsSlots.Length())));
#endif
			for (UINT32 BlendIndex = 0u, BlendNum = BlendIndicesSlots.Length(); BlendIndex < BlendNum; BlendIndex++)
			{
				UINT32 BlendIndicesSlot = BlendIndicesSlots[BlendIndex];
				UINT32 BlendWeightsSlot = BlendWeightsSlots[BlendIndex];
#if !(_TEST_MODE || _SHIPPING_MODE)
				PE_CHECK((ENGINE_ASSET_ERROR), ("slot of [BlendIndices] and slot of [BlendWeight] is not matched."), (BlendIndicesSlot == BlendWeightsSlot));
#else
				if (BlendIndicesSlot != BlendWeightsSlot)
				{
					continue;
				}
#endif
				Result.Add(StoredMeshLayoutDesc());
				StoredMeshLayoutDesc& CurrentDesc = Result[Result.Length() - 1u];
				CurrentDesc.BaseVertexLayout = EVertexLayoutType::MESH_SKIN;
				CurrentDesc.TryStoredLayoutNum += 1u;
				CurrentDesc.TryStoredLayoutSlot[CurrentDesc.TryStoredLayoutNum - 1u] = BlendIndicesSlot;
			}
		}
		return Result;
	}
	static EIndexData* TranslateAssimpMeshFaceToEngineMeshIndices(const aiMesh* InMesh)
	{
		const UINT32 ElementNum = InMesh->mNumFaces * 3u;
		const UINT32 IndexStride = ((InMesh->mNumVertices) > 0xffffu) ? 4u : 2u;	// Index stride is integer_32bit or integer_16bit
		EIndexData* OutMeshIndices = new EIndexData(IndexStride, ElementNum);
		if (!(OutMeshIndices->Indices))
		{
			OutMeshIndices->Indices = new UINT32[ElementNum];
		}
		for (UINT32 FaceIndex = 0u, FaceNum = InMesh->mNumFaces; FaceIndex < FaceNum; FaceIndex++)
		{
			const aiFace& TempFace = InMesh->mFaces[FaceIndex];
			PE_CHECK((ENGINE_ASSET_ERROR), ("We must import mesh that only consist of triangles."), (TempFace.mNumIndices == 3u));
			for (UINT32 TriangleIndex = 0u; TriangleIndex < 3u; TriangleIndex++)
			{
				if ((FaceIndex * 3u + TriangleIndex) < ElementNum)
				{
#if _EDITOR_ONLY
					if (IndexStride == 2u)
					{
						PE_CHECK((ENGINE_ASSET_ERROR), ("Half type index buffer can not greater than 65535u."), ((TempFace.mIndices[TriangleIndex]) <= 0xffffu));
					}
#endif
					OutMeshIndices->Indices[FaceIndex * 3u + TriangleIndex] = TempFace.mIndices[TriangleIndex];
				}
			}
		}
		return OutMeshIndices;
	}
	static void CombineEngineMeshIntoSubmeshes(const TArray<EStaticMesh>& InMeshes, EStaticMesh& OutMesh)
	{
		if (InMeshes.Length() == 0u)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("There is no static meshes for combine."));
			return;
		}

		if (InMeshes.Length() == 1u)
		{
			OutMesh = InMeshes[0];
			return;
		}

		constexpr UINT32 VertexLayoutTypes[6] =
		{
			EVertexLayoutType::MESH_VERTEX,
			EVertexLayoutType::MESH_TEXTURECOORD,
			EVertexLayoutType::MESH_NORMAL,
			EVertexLayoutType::MESH_TANGENT,
			EVertexLayoutType::MESH_COLOR,
			EVertexLayoutType::MESH_BITANGENT
		};
		constexpr UINT32 VertexLayoutIndices[6] =
		{
			2u,
			6u,
			10u,
			14u,
			18u,
			22u
		};
		constexpr UINT32 VertexLayout32BitsNum[6] =
		{
			4u,
			2u,
			4u,
			4u,
			4u,
			4u
		};
		constexpr UINT32 VertexLayoutStrides[6] =
		{
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 2u,
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 4u
		};
		UINT32 MeshVertexLayoutNum[6] = { 0u, 0u, 0u, 0u, 0u, 0u };
		UINT32 NewMeshVertexNum = 0u, NewMeshIndexNum = 0u;
		TArray<ESubmeshData> NewMeshSubmeshDatas;
		for (UINT32 MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
		{
			const EStaticMesh&			UsedMesh				= InMeshes[MeshIndex];
			const EMesh::EVertexPart&	VertexPart				= UsedMesh.GetVertexPart();
			const EMesh::EIndexPart&	IndexPart				= UsedMesh.GetIndexPart();
			const UINT32				UsedMeshVertexLayout	= UsedMesh.GetVertexLayout();
			Check((UsedMeshVertexLayout != 0x0u), (ENGINE_ASSET_ERROR));
			Check(((IndexPart.ElementNum > 0u) && ((IndexPart.Stride == 2u) || (IndexPart.Stride == 4u))), (ENGINE_ASSET_ERROR));
			Check((VertexPart.Length() > 0u), (ENGINE_ASSET_ERROR));
			UINT32 TempMeshVertexLayoutNum = 0u;
			for (UINT32 MeshVertexLayoutIndex = 0u; MeshVertexLayoutIndex < 6u; MeshVertexLayoutIndex++)
			{
				MeshVertexLayoutNum[MeshVertexLayoutIndex] = (MeshVertexLayoutNum[MeshVertexLayoutIndex]) | ((UsedMeshVertexLayout & (0xfu << (VertexLayoutIndices[MeshVertexLayoutIndex]))) >> (VertexLayoutIndices[MeshVertexLayoutIndex]));
			}
			ESubmeshData& NewSubmeshData = NewMeshSubmeshDatas.Add_Default_GetRef();
			NewSubmeshData.StartIndex	= NewMeshIndexNum;
			NewSubmeshData.IndexNum		= IndexPart.ElementNum;
			NewSubmeshData.StartVertex	= NewMeshVertexNum;
			NewSubmeshData.VertexNum	= VertexPart[0].ElementNum;
			NewMeshVertexNum	= NewMeshVertexNum + VertexPart[0].ElementNum;
			NewMeshIndexNum		= NewMeshIndexNum + IndexPart.ElementNum;
		}
		UINT32 NewMeshIndexStride = 2u;
		if (NewMeshVertexNum > 65536u)
		{
			Check((NewMeshVertexNum <= 0xffffffffu), (ENGINE_ASSET_ERROR));
			NewMeshIndexStride = 4u;
		}
		EIndexData NewIndexData;
		TArray<EVertexData> NewVertexDatas;
		{
			{
				NewIndexData.PartType	= (NewMeshIndexStride == 4u) ? EVertexLayoutType::MESH_INDEX_FULL : EVertexLayoutType::MESH_INDEX_HALF;
				NewIndexData.ElementNum	= NewMeshIndexNum;
				NewIndexData.Stride		= NewMeshIndexStride;
				NewIndexData.Indices	= new UINT32[NewMeshIndexNum];
			}
			{
				for (UINT32 LayoutIndex = 0u; LayoutIndex < 6u; LayoutIndex++)
				{
					for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
					{
						EVertexData& NewVertexData	= NewVertexDatas.Add_Default_GetRef();
						NewVertexData.PartType		= VertexLayoutTypes[LayoutIndex];
						NewVertexData.ElementNum	= NewMeshVertexNum;
						NewVertexData.Stride		= 0u;
						NewVertexData.Datas			= nullptr;
						if ((MeshVertexLayoutNum[LayoutIndex] & (0x1u << LayoutPartIndex)) != 0u)
						{
							NewVertexData.Stride	= VertexLayoutStrides[LayoutIndex];
							NewVertexData.Datas		= new FLOAT[(VertexLayoutStrides[LayoutIndex] / sizeof(FLOAT)) * NewMeshVertexNum];
							::memset(NewVertexData.Datas, 0, VertexLayoutStrides[LayoutIndex] * NewMeshVertexNum);
						}
					}
				}
			}
			for (UINT32 MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
			{
				const EStaticMesh&	UsedMesh		= InMeshes[MeshIndex];
				const ESubmeshData&	NewSubmeshData	= NewMeshSubmeshDatas[MeshIndex];

				// Index part append
				const EMesh::EIndexPart& IndexPart = UsedMesh.GetIndexPart();
				Check((NewSubmeshData.IndexNum == IndexPart.ElementNum), (ENGINE_ASSET_ERROR));
				for (UINT32 i = 0u, n = IndexPart.ElementNum; i < n; i++)
				{
					NewIndexData.Indices[NewSubmeshData.StartIndex + i] = IndexPart.Indices[i] + NewSubmeshData.StartVertex;
				}

				// Vertex part append
				const EMesh::EVertexPart& VertexPart = UsedMesh.GetVertexPart();
				Check((VertexPart.Length() > 0u), (ENGINE_ASSET_ERROR));
				Check((NewSubmeshData.VertexNum == VertexPart[0].ElementNum), (ENGINE_ASSET_ERROR));
				for (UINT32 LayoutIndex = 0u; LayoutIndex < 6u; LayoutIndex++)
				{
					for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
					{
						EVertexData& NewVertexData = NewVertexDatas[LayoutIndex * 4u + LayoutPartIndex];
						if (!!(NewVertexData.Datas))
						{
							const EVertexData* OldVertexData = nullptr;
							if (UsedMesh.GetVertexElement(VertexLayoutTypes[LayoutIndex], LayoutPartIndex, OldVertexData))
							{
								Check((NewVertexData.Stride == OldVertexData->Stride), (ENGINE_ASSET_ERROR));
								for (UINT32 i = 0u, n = OldVertexData->ElementNum; i < n; i++)
								{
									for (UINT32 j = 0u; j < VertexLayout32BitsNum[LayoutIndex]; j++)
									{
										NewVertexData.Datas[(NewSubmeshData.StartVertex + i) * VertexLayout32BitsNum[LayoutIndex] + j] = OldVertexData->Datas[i * VertexLayout32BitsNum[LayoutIndex] + j];
									}
								}
							}
						}
					}
				}
			}
		}
		OutMesh.AddIndexElement(&NewIndexData);
		for (UINT32 LayoutIndex = 0u; LayoutIndex < 6u; LayoutIndex++)
		{
			for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
			{
				EVertexData& NewVertexData = NewVertexDatas[LayoutIndex * 4u + LayoutPartIndex];
				if (!!(NewVertexData.Datas))
				{
#if _EDITOR_ONLY
					BOOL32 Result =
#endif
						OutMesh.AddVertexElement(&NewVertexData);
#if _EDITOR_ONLY
					Check((Result), (ENGINE_ASSET_ERROR));
#endif
				}
			}
		}
		for (UINT32 SubmeshIndex = 0u, SubmeshNum = NewMeshSubmeshDatas.Length(); SubmeshIndex < SubmeshNum; SubmeshIndex++)
		{
			ESubmeshData& NewMeshSubmeshData = NewMeshSubmeshDatas[SubmeshIndex];
			OutMesh.AddSubmesh(&NewMeshSubmeshData);
		}
	}
	static void CombineEngineMeshIntoSubmeshes(const TArray<ESkinnedMesh>& InMeshes, const USHORT InMaxBoneWeightNum, ESkinnedMesh& OutMesh)
	{
		if (InMeshes.Length() == 0u)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("There is no skinned meshes for combine."));
			return;
		}

		Check(((InMaxBoneWeightNum > 0u) && (InMaxBoneWeightNum <= EEngineSettings::ENGINE_BONE_WEIGHT_NUM_MAXIMUM)), (ENGINE_ASSET_ERROR));

		if (InMeshes.Length() == 1u)
		{
			OutMesh = InMeshes[0];
			return;
		}

		constexpr UINT32 VertexLayoutTypes[7] =
		{
			EVertexLayoutType::MESH_VERTEX,
			EVertexLayoutType::MESH_TEXTURECOORD,
			EVertexLayoutType::MESH_NORMAL,
			EVertexLayoutType::MESH_TANGENT,
			EVertexLayoutType::MESH_COLOR,
			EVertexLayoutType::MESH_BITANGENT,
			EVertexLayoutType::MESH_SKIN
		};
		constexpr UINT32 VertexLayoutIndices[7] =
		{
			2u,
			6u,
			10u,
			14u,
			18u,
			22u,
			26u
		};
		constexpr UINT32 VertexLayout32BitsNum[6] =
		{
			4u,
			2u,
			4u,
			4u,
			4u,
			4u
		};
		const UINT32 VertexLayoutStrides[7] =
		{
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 2u,
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 4u,
			sizeof(FLOAT) * 4u,
			InMaxBoneWeightNum * sizeof(FLOAT)
		};
		UINT32 MeshVertexLayoutNum[7] = { 0u, 0u, 0u, 0u, 0u, 0u, 0u };
		UINT32 NewMeshVertexNum = 0u, NewMeshIndexNum = 0u;
		TArray<ESubmeshData> NewMeshSubmeshDatas;
		for (UINT32 MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
		{
			const ESkinnedMesh&				UsedMesh				= InMeshes[MeshIndex];
			const EMesh::EVertexPart&		VertexPart				= UsedMesh.GetVertexPart();
			const EMesh::EIndexPart&		IndexPart				= UsedMesh.GetIndexPart();
#if _EDITOR_ONLY
			const ESkinnedMesh::ESkinPart&	SkinPart				= UsedMesh.GetSkinPart();
#endif
			const UINT32					UsedMeshVertexLayout	= UsedMesh.GetVertexLayout();
			Check((UsedMeshVertexLayout != 0x0u), (ENGINE_ASSET_ERROR));
			Check(((IndexPart.ElementNum > 0u) && ((IndexPart.Stride == 2u) || (IndexPart.Stride == 4u))), (ENGINE_ASSET_ERROR));
			Check((VertexPart.Length() > 0u), (ENGINE_ASSET_ERROR));
#if _EDITOR_ONLY
			Check((SkinPart.Length() > 0u), (ENGINE_ASSET_ERROR));
			Check((VertexPart[0].ElementNum == SkinPart[0].ElementNum), (ENGINE_ASSET_ERROR));
#endif
			UINT32 TempMeshVertexLayoutNum = 0u;
			for (UINT32 MeshVertexLayoutIndex = 0u; MeshVertexLayoutIndex < 7u; MeshVertexLayoutIndex++)
			{
				MeshVertexLayoutNum[MeshVertexLayoutIndex] = (MeshVertexLayoutNum[MeshVertexLayoutIndex]) | ((UsedMeshVertexLayout & (0xfu << (VertexLayoutIndices[MeshVertexLayoutIndex]))) >> (VertexLayoutIndices[MeshVertexLayoutIndex]));
			}
			ESubmeshData& NewSubmeshData = NewMeshSubmeshDatas.Add_Default_GetRef();
			NewSubmeshData.StartIndex	= NewMeshIndexNum;
			NewSubmeshData.IndexNum		= IndexPart.ElementNum;
			NewSubmeshData.StartVertex	= NewMeshVertexNum;
			NewSubmeshData.VertexNum	= VertexPart[0].ElementNum;
			NewMeshVertexNum	= NewMeshVertexNum + VertexPart[0].ElementNum;
			NewMeshIndexNum		= NewMeshIndexNum + IndexPart.ElementNum;

			const ESkinnedMesh::EBindPoseIndex& BindPoseIndexPart = UsedMesh.GetBindPoseIndex();
			const ESkinnedMesh::EBindPoseValue& BindPoseValuePart = UsedMesh.GetBindPoseValue();
			Check(((BindPoseIndexPart.Length() > 0u) && (BindPoseIndexPart.Length() == BindPoseValuePart.Length())), (ENGINE_ASSET_ERROR));
			for (auto It = BindPoseIndexPart.Begin(); It != BindPoseIndexPart.End(); It++)
			{
				if (const Matrix4x4* BindPoseValuePtr = BindPoseValuePart.FindValueAsPtr(It->first); !!BindPoseValuePtr)
				{
					OutMesh.AddBindPose(It->first, (*BindPoseValuePtr));
				}
			}
		}

		OutMesh.GenerateBindPoseIndex();
		OutMesh.SetEffectBoneNum(InMaxBoneWeightNum);

		UINT32 NewMeshIndexStride = 2u;
		if (NewMeshVertexNum > 65536u)
		{
			Check((NewMeshVertexNum <= 0xffffffffu), (ENGINE_ASSET_ERROR));
			NewMeshIndexStride = 4u;
		}
		EIndexData NewIndexData;
		TArray<EVertexData> NewVertexDatas;
		TArray<ESkinData> NewSkinDatas;
		{
			{
				NewIndexData.PartType	= (NewMeshIndexStride == 4u) ? EVertexLayoutType::MESH_INDEX_FULL : EVertexLayoutType::MESH_INDEX_HALF;
				NewIndexData.ElementNum = NewMeshIndexNum;
				NewIndexData.Stride		= NewMeshIndexStride;
				NewIndexData.Indices	= new UINT32[NewMeshIndexNum];
			}
			{
				for (UINT32 LayoutIndex = 0u; LayoutIndex < 6u; LayoutIndex++)
				{
					for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
					{
						EVertexData& NewVertexData	= NewVertexDatas.Add_Default_GetRef();
						NewVertexData.PartType		= VertexLayoutTypes[LayoutIndex];
						NewVertexData.ElementNum	= NewMeshVertexNum;
						NewVertexData.Stride		= 0u;
						NewVertexData.Datas			= nullptr;
						if ((MeshVertexLayoutNum[LayoutIndex] & (0x1u << LayoutPartIndex)) != 0u)
						{
							NewVertexData.Stride = VertexLayoutStrides[LayoutIndex];
							NewVertexData.Datas = new FLOAT[(VertexLayoutStrides[LayoutIndex] / sizeof(FLOAT)) * NewMeshVertexNum];
							::memset(NewVertexData.Datas, 0, VertexLayoutStrides[LayoutIndex] * NewMeshVertexNum);
						}
					}
				}
				for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
				{
					ESkinData& NewSkinData	= NewSkinDatas.Add_Default_GetRef();
					NewSkinData.PartType	= VertexLayoutTypes[6];
					NewSkinData.ElementNum	= NewMeshVertexNum;
					NewSkinData.Stride		= 0u;
					NewSkinData.Indices		= nullptr;
					NewSkinData.Weights		= nullptr;
					if ((MeshVertexLayoutNum[6] & (0x1u << LayoutPartIndex)) != 0u)
					{
						NewSkinData.Stride = VertexLayoutStrides[6];
						NewSkinData.Indices = new UINT32[InMaxBoneWeightNum * NewMeshVertexNum];
						NewSkinData.Weights = new FLOAT[InMaxBoneWeightNum * NewMeshVertexNum];
						::memset(NewSkinData.Indices, 0, InMaxBoneWeightNum * 4u * NewMeshVertexNum);
						::memset(NewSkinData.Weights, 0, InMaxBoneWeightNum * 4u * NewMeshVertexNum);
					}
				}
			}
			const ESkinnedMesh::EBindPoseIndex& NewBindPoseIndexPart = OutMesh.GetBindPoseIndex();
			for (UINT32 MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
			{
				const ESkinnedMesh& UsedMesh = InMeshes[MeshIndex];
				const ESubmeshData& NewSubmeshData = NewMeshSubmeshDatas[MeshIndex];

				// Index part append
				const EMesh::EIndexPart& IndexPart = UsedMesh.GetIndexPart();
				Check((NewSubmeshData.IndexNum == IndexPart.ElementNum), (ENGINE_ASSET_ERROR));
				for (UINT32 i = 0u, n = IndexPart.ElementNum; i < n; i++)
				{
					NewIndexData.Indices[NewSubmeshData.StartIndex + i] = IndexPart.Indices[i] + NewSubmeshData.StartVertex;
				}

				// Vertex part append
				const EMesh::EVertexPart& VertexPart = UsedMesh.GetVertexPart();
				Check((VertexPart.Length() > 0u), (ENGINE_ASSET_ERROR));
				Check((NewSubmeshData.VertexNum == VertexPart[0].ElementNum), (ENGINE_ASSET_ERROR));
				for (UINT32 LayoutIndex = 0u; LayoutIndex < 6u; LayoutIndex++)
				{
					for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
					{
						EVertexData& NewVertexData = NewVertexDatas[LayoutIndex * 4u + LayoutPartIndex];
						if (!!(NewVertexData.Datas))
						{
							const EVertexData* OldVertexData = nullptr;
							if (UsedMesh.GetVertexElement(VertexLayoutTypes[LayoutIndex], LayoutPartIndex, OldVertexData))
							{
								Check((NewVertexData.Stride == OldVertexData->Stride), (ENGINE_ASSET_ERROR));
								for (UINT32 i = 0u, n = OldVertexData->ElementNum; i < n; i++)
								{
									for (UINT32 j = 0u; j < VertexLayout32BitsNum[LayoutIndex]; j++)
									{
										NewVertexData.Datas[(NewSubmeshData.StartVertex + i) * VertexLayout32BitsNum[LayoutIndex] + j] = OldVertexData->Datas[i * VertexLayout32BitsNum[LayoutIndex] + j];
									}
								}
							}
						}
					}
				}

				// Skin part append
				const ESkinnedMesh::EBindPoseIndex& BindPoseIndexPart = UsedMesh.GetBindPoseIndex();
				TMap<USHORT, EString> TempBindPoseIndexPart;
				for (auto It = BindPoseIndexPart.Begin(); It != BindPoseIndexPart.End(); It++)
				{
					TempBindPoseIndexPart.Add(It->second, It->first);
				}
				const ESkinnedMesh::ESkinPart& SkinPart = UsedMesh.GetSkinPart();
				Check((SkinPart.Length() > 0u), (ENGINE_ASSET_ERROR));
				Check((NewSubmeshData.VertexNum == SkinPart[0].ElementNum), (ENGINE_ASSET_ERROR));
				for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
				{
					ESkinData& NewSkinData = NewSkinDatas[LayoutPartIndex];
					if ((!!(NewSkinData.Indices)) && (!!(NewSkinData.Weights)))
					{
						const ESkinData* OldSkinData = nullptr;
						if (UsedMesh.GetSkinElement(LayoutPartIndex, OldSkinData))
						{
							Check((InMaxBoneWeightNum >= UsedMesh.GetEffectBoneNum()), (ENGINE_ASSET_ERROR));
							Check((UsedMesh.GetEffectBoneNum() == (OldSkinData->Stride / sizeof(FLOAT))), (ENGINE_ASSET_ERROR));
							for (UINT32 i = 0u, n = OldSkinData->ElementNum; i < n; i++)
							{
								for (USHORT j = 0u, k = UsedMesh.GetEffectBoneNum(); j < k; j++)
								{
									USHORT OldIndices = OldSkinData->Indices[i * k + j];
									FLOAT OldWeights = OldSkinData->Weights[i * k + j];
									if (const EString* BoneNamePtr = TempBindPoseIndexPart.FindValueAsPtr(OldIndices); !!BoneNamePtr)
									{
										if (const USHORT* NewBoneIndexPtr = NewBindPoseIndexPart.FindValueAsPtr(*BoneNamePtr); !!NewBoneIndexPtr)
										{
											NewSkinData.Indices[(NewSubmeshData.StartVertex + i) * InMaxBoneWeightNum + j] = (*NewBoneIndexPtr);
											NewSkinData.Weights[(NewSubmeshData.StartVertex + i) * InMaxBoneWeightNum + j] = OldWeights;
										}
									}
								}
							}
						}
#if _EDITOR_ONLY
						else
						{
							EString ErrorInfo("Skin does not contain this[");
							ErrorInfo = ErrorInfo + ToString(LayoutPartIndex) + ("] skin data.");
							PE_FAILED((ENGINE_ASSET_ERROR), (*ErrorInfo));
						}
#endif
					}
				}
			}
		}
		OutMesh.AddIndexElement(&NewIndexData);
		for (UINT32 LayoutIndex = 0u; LayoutIndex < 6u; LayoutIndex++)
		{
			for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
			{
				EVertexData& NewVertexData = NewVertexDatas[LayoutIndex * 4u + LayoutPartIndex];
				if (!!(NewVertexData.Datas))
				{
#if _EDITOR_ONLY
					BOOL32 Result =
#endif
						OutMesh.AddVertexElement(&NewVertexData);
#if _EDITOR_ONLY
					Check((Result), (ENGINE_ASSET_ERROR));
#endif
				}
			}
		}
		for (UINT32 LayoutPartIndex = 0u; LayoutPartIndex < 4u; LayoutPartIndex++)
		{
			ESkinData& NewSkinData = NewSkinDatas[LayoutPartIndex];
			if ((!!(NewSkinData.Indices)) && (!!(NewSkinData.Weights)))
			{
#if _EDITOR_ONLY
				BOOL32 Result =
#endif
					OutMesh.AddSkinElement(&NewSkinData);
#if _EDITOR_ONLY
				Check((Result), (ENGINE_ASSET_ERROR));
#endif
			}
		}
		for (UINT32 SubmeshIndex = 0u, SubmeshNum = NewMeshSubmeshDatas.Length(); SubmeshIndex < SubmeshNum; SubmeshIndex++)
		{
			ESubmeshData& NewMeshSubmeshData = NewMeshSubmeshDatas[SubmeshIndex];
			OutMesh.AddSubmesh(&NewMeshSubmeshData);
		}
	}
	static void TranslateAssimpMeshToEngineMeshInternal(const RShaderSemanticType* InEngineLayouts, const UINT32 InEngineLayoutNum, const TArray<const aiMesh*>& InMeshes, TArray<EStaticMesh>& OutMeshes)
	{
		TArray<StoredMeshLayoutDesc> ShouldStoredLayoutDescriptions = GetShouldStoredMeshLayoutDescriptions(InEngineLayouts, InEngineLayoutNum);
		for (UINT32 MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
		{
			const aiMesh* AssimpMesh = InMeshes[MeshIndex];
			if ((!AssimpMesh) || ((AssimpMesh->mNumVertices) < 3u) || (!(AssimpMesh->HasFaces())) || ((AssimpMesh->mNumFaces) < 3u))
			{
				continue;
			}
			OutMeshes.Add(EStaticMesh(AssimpTranslateString(AssimpMesh->mName)));
			EStaticMesh& OutMesh = OutMeshes[OutMeshes.Length() - 1u];
			{
				OutMesh.SetBoundAABB(AssimpTranslateVector3(AssimpMesh->mAABB.mMin), AssimpTranslateVector3(AssimpMesh->mAABB.mMax));

				ESubmeshData NewSubmeshData;
				NewSubmeshData.StartIndex = 0u;
				NewSubmeshData.StartVertex = 0u;

				EIndexData* MeshIndices = TranslateAssimpMeshFaceToEngineMeshIndices(AssimpMesh);
				NewSubmeshData.IndexNum = MeshIndices->ElementNum;
				if (!(OutMesh.AddIndexElement(MeshIndices)))
				{
					MeshIndices->Release();
					delete MeshIndices;
				}

				NewSubmeshData.VertexNum = AssimpMesh->mNumVertices;

				OutMesh.AddSubmesh(&NewSubmeshData);
			}
			for (UINT32 LayoutDescIndex = 0u, LayoutDescNum = ShouldStoredLayoutDescriptions.Length(); LayoutDescIndex < LayoutDescNum; LayoutDescIndex++)
			{
				StoredMeshLayoutDesc& StoredLayoutDesc = ShouldStoredLayoutDescriptions[LayoutDescIndex];
				EVertexLayoutType TempTryStoredLayoutBaseType = StoredLayoutDesc.BaseVertexLayout;
				if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_VERTEX) != 0u)
				{
					const UINT32 SuccessAddPositionMaxNum = 1u;
					const UINT32 PositionStrideIn32Bits = 4u;
					TArray<const aiVector3D*> TempPositionDatas;
					TArray<UINT32> TempPositionNum;
					TempPositionDatas.Add(AssimpMesh->mVertices);
					TempPositionNum.Add(AssimpMesh->mNumVertices);
					TryAddMeshVertexPart(TempPositionDatas, TempPositionNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, PositionStrideIn32Bits, SuccessAddPositionMaxNum, OutMesh);
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_TEXTURECOORD) != 0u)
				{
					if (AssimpMesh->GetNumUVChannels() > 0)
					{
						const UINT32 SuccessAddTexcoordMaxNum = 3u;
						const UINT32 TexcoordStrideIn32Bits = 2u;
						TArray<const aiVector3D*> TempTexcoordDatas;
						TArray<UINT32> TempTexcoordNum;
						for (UINT32 TexcoordIndex = 0u, TexcoordMax = AI_MAX_NUMBER_OF_TEXTURECOORDS; TexcoordIndex < TexcoordMax; TexcoordIndex++)
						{
							if (!(AssimpMesh->HasTextureCoords(TexcoordIndex)))
							{
								continue;
							}
							TempTexcoordDatas.Add(AssimpMesh->mTextureCoords[TexcoordIndex]);
							TempTexcoordNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempTexcoordDatas, TempTexcoordNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TexcoordStrideIn32Bits, SuccessAddTexcoordMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_NORMAL) != 0u)
				{
					if (AssimpMesh->HasNormals())
					{
						const UINT32 SuccessAddNormalMaxNum = 1u;
						const UINT32 NormalStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempNormalDatas;
						TArray<UINT32> TempNormalNum;
						TempNormalDatas.Add(AssimpMesh->mNormals);
						TempNormalNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempNormalDatas, TempNormalNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, NormalStrideIn32Bits, SuccessAddNormalMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_TANGENT) != 0u)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT32 SuccessAddTangentMaxNum = 1u;
						const UINT32 TangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempTangentDatas;
						TArray<UINT32> TempTangentNum;
						TempTangentDatas.Add(AssimpMesh->mTangents);
						TempTangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempTangentDatas, TempTangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TangentStrideIn32Bits, SuccessAddTangentMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_COLOR) != 0u)
				{
					if (AssimpMesh->GetNumColorChannels() > 0)
					{
						const UINT32 SuccessAddColorMaxNum = 1u;
						const UINT32 ColorStrideIn32Bits = 4u;
						TArray<const aiColor4D*> TempColorDatas;
						TArray<UINT32> TempColorNum;
						for (UINT32 ColorIndex = 0u, ColorMax = AI_MAX_NUMBER_OF_COLOR_SETS; ColorIndex < ColorMax; ColorIndex++)
						{
							if (!(AssimpMesh->HasVertexColors(ColorIndex)))
							{
								continue;
							}
							TempColorDatas.Add(AssimpMesh->mColors[ColorIndex]);
							TempColorNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempColorDatas, TempColorNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, ColorStrideIn32Bits, SuccessAddColorMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_BITANGENT) != 0u)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT32 SuccessAddBitangentMaxNum = 1u;
						const UINT32 BitangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempBitangentDatas;
						TArray<UINT32> TempBitangentNum;
						TempBitangentDatas.Add(AssimpMesh->mBitangents);
						TempBitangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempBitangentDatas, TempBitangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, BitangentStrideIn32Bits, SuccessAddBitangentMaxNum, OutMesh);
					}
				}
			}
		}
	}
	static void TranslateAssimpMeshToEngineMeshInternal(const RShaderSemanticType* InEngineLayouts, const UINT32 InEngineLayoutNum, const TArray<const aiMesh*>& InMeshes, TArray<ESkinnedMesh>& OutMeshes)
	{
		TArray<StoredMeshLayoutDesc> ShouldStoredLayoutDescriptions = GetShouldStoredMeshLayoutDescriptions(InEngineLayouts, InEngineLayoutNum);
		for (UINT32 MeshIndex = 0u, MeshNum = InMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
		{
			const aiMesh* AssimpMesh = InMeshes[MeshIndex];
			if ((!AssimpMesh) || ((AssimpMesh->mNumVertices) < 3u) || (!(AssimpMesh->HasFaces())) || ((AssimpMesh->mNumFaces) < 3u))
			{
				continue;
			}
			OutMeshes.Add(ESkinnedMesh(AssimpTranslateString(AssimpMesh->mName)));
			ESkinnedMesh& OutMesh = OutMeshes[OutMeshes.Length() - 1u];
			{
				OutMesh.SetBoundAABB(AssimpTranslateVector3(AssimpMesh->mAABB.mMin), AssimpTranslateVector3(AssimpMesh->mAABB.mMax));

				ESubmeshData NewSubmeshData;
				NewSubmeshData.StartIndex = 0u;
				NewSubmeshData.StartVertex = 0u;

				EIndexData* MeshIndices = TranslateAssimpMeshFaceToEngineMeshIndices(AssimpMesh);
				NewSubmeshData.IndexNum = MeshIndices->ElementNum;
				if (!(OutMesh.AddIndexElement(MeshIndices)))
				{
					MeshIndices->Release();
					delete MeshIndices;
				}

				NewSubmeshData.VertexNum = AssimpMesh->mNumVertices;

				OutMesh.AddSubmesh(&NewSubmeshData);
			}
			for (UINT32 LayoutDescIndex = 0u, LayoutDescNum = ShouldStoredLayoutDescriptions.Length(); LayoutDescIndex < LayoutDescNum; LayoutDescIndex++)
			{
				StoredMeshLayoutDesc& StoredLayoutDesc = ShouldStoredLayoutDescriptions[LayoutDescIndex];
				EVertexLayoutType TempTryStoredLayoutBaseType = StoredLayoutDesc.BaseVertexLayout;
				if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_VERTEX) != 0u)
				{
					const UINT32 SuccessAddPositionMaxNum = 1u;
					const UINT32 PositionStrideIn32Bits = 4u;
					TArray<const aiVector3D*> TempPositionDatas;
					TArray<UINT32> TempPositionNum;
					TempPositionDatas.Add(AssimpMesh->mVertices);
					TempPositionNum.Add(AssimpMesh->mNumVertices);
					TryAddMeshVertexPart(TempPositionDatas, TempPositionNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, PositionStrideIn32Bits, SuccessAddPositionMaxNum, OutMesh);
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_TEXTURECOORD) != 0u)
				{
					if (AssimpMesh->GetNumUVChannels() > 0)
					{
						const UINT32 SuccessAddTexcoordMaxNum = 3u;
						const UINT32 TexcoordStrideIn32Bits = 2u;
						TArray<const aiVector3D*> TempTexcoordDatas;
						TArray<UINT32> TempTexcoordNum;
						for (UINT32 TexcoordIndex = 0u, TexcoordMax = AI_MAX_NUMBER_OF_TEXTURECOORDS; TexcoordIndex < TexcoordMax; TexcoordIndex++)
						{
							if (!(AssimpMesh->HasTextureCoords(TexcoordIndex)))
							{
								continue;
							}
							TempTexcoordDatas.Add(AssimpMesh->mTextureCoords[TexcoordIndex]);
							TempTexcoordNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempTexcoordDatas, TempTexcoordNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TexcoordStrideIn32Bits, SuccessAddTexcoordMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_NORMAL) != 0u)
				{
					if (AssimpMesh->HasNormals())
					{
						const UINT32 SuccessAddNormalMaxNum = 1u;
						const UINT32 NormalStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempNormalDatas;
						TArray<UINT32> TempNormalNum;
						TempNormalDatas.Add(AssimpMesh->mNormals);
						TempNormalNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempNormalDatas, TempNormalNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, NormalStrideIn32Bits, SuccessAddNormalMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_TANGENT) != 0u)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT32 SuccessAddTangentMaxNum = 1u;
						const UINT32 TangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempTangentDatas;
						TArray<UINT32> TempTangentNum;
						TempTangentDatas.Add(AssimpMesh->mTangents);
						TempTangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempTangentDatas, TempTangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, TangentStrideIn32Bits, SuccessAddTangentMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_COLOR) != 0u)
				{
					if (AssimpMesh->GetNumColorChannels() > 0)
					{
						const UINT32 SuccessAddColorMaxNum = 1u;
						const UINT32 ColorStrideIn32Bits = 4u;
						TArray<const aiColor4D*> TempColorDatas;
						TArray<UINT32> TempColorNum;
						for (UINT32 ColorIndex = 0u, ColorMax = AI_MAX_NUMBER_OF_COLOR_SETS; ColorIndex < ColorMax; ColorIndex++)
						{
							if (!(AssimpMesh->HasVertexColors(ColorIndex)))
							{
								continue;
							}
							TempColorDatas.Add(AssimpMesh->mColors[ColorIndex]);
							TempColorNum.Add(AssimpMesh->mNumVertices);
						}
						TryAddMeshVertexPart(TempColorDatas, TempColorNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, ColorStrideIn32Bits, SuccessAddColorMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_BITANGENT) != 0u)
				{
					if (AssimpMesh->HasTangentsAndBitangents())
					{
						const UINT32 SuccessAddBitangentMaxNum = 1u;
						const UINT32 BitangentStrideIn32Bits = 4u;
						TArray<const aiVector3D*> TempBitangentDatas;
						TArray<UINT32> TempBitangentNum;
						TempBitangentDatas.Add(AssimpMesh->mBitangents);
						TempBitangentNum.Add(AssimpMesh->mNumVertices);
						TryAddMeshVertexPart(TempBitangentDatas, TempBitangentNum, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, BitangentStrideIn32Bits, SuccessAddBitangentMaxNum, OutMesh);
					}
				}
				else if ((TempTryStoredLayoutBaseType & EVertexLayoutType::MESH_SKIN) != 0u)
				{
					if (AssimpMesh->HasBones())
					{
						const UINT32 SuccessAddBoneMaxNum = 1u;
						TArray<const aiBone*> TempBones;
						for (UINT32 BoneIndex = 0u; BoneIndex < AssimpMesh->mNumBones; BoneIndex++)
						{
							const aiBone* TempBone = AssimpMesh->mBones[BoneIndex];
							if (!TempBone)
							{
								continue;
							}
							if (TempBone->mNumWeights > 0u)
							{
								TempBones.Add(TempBone);
							}
						}
						TryAddMeshSkinPart(TempBones, AssimpMesh->mNumVertices, TempTryStoredLayoutBaseType, StoredLayoutDesc.TryStoredLayoutSlot, StoredLayoutDesc.TryStoredLayoutNum, SuccessAddBoneMaxNum, OutMesh);
					}
				}
			}
		}
	}
	static BOOL32 GatherAllBoneNodeDatas(const aiScene* InScene, TArray<EBoneData>& OutBones, TMap<EString, SHORT>& OutBoneIndices)
	{
		if (OutBones.Length() > 0u)
		{
			OutBones.Clear();
		}

		// Read data structures of all nodes.
		TArray<const aiNode*> TempNodes; TMap<const aiNode*, EString> TempNodeNames; TMap<EString, SHORT> TempNodeIndices;
		AssimpGatherSingleNodeRecursion(InScene->mRootNode, TempNodes, TempNodeNames, TempNodeIndices);

		// Check read state.
		if ((TempNodes.Length() < 1u) || (TempNodes.Length() != TempNodeNames.Length()) || (TempNodes.Length() != TempNodeIndices.Length()))
		{
			return FALSE;
		}

		// Create engine's hierarchical bones' data.
		for (UINT32 NodeIndex = 0u, NodeNum = TempNodes.Length(); NodeIndex < NodeNum; NodeIndex++)
		{
			EString TempNodeName;
			if (!(TempNodeNames.FindValue(TempNodes[NodeIndex], TempNodeName)))
			{
				PE_FAILED((ENGINE_ASSET_ERROR), ("Bone node find name failed(This step must be finded)."));
				continue;
			}
			OutBones.Add(EBoneData(TempNodeName));
			EBoneData& NewBone = OutBones[OutBones.Length() - 1u];
			NewBone.Index = static_cast<SHORT>(NodeIndex);
			NewBone.Parent = -1;
			if (TempNodes[NodeIndex]->mParent != nullptr)
			{
				EString TempNodeParentName;
				if (TempNodeNames.FindValue(TempNodes[NodeIndex]->mParent, TempNodeParentName))
				{
					SHORT TempNodeParentIndex = -1;
					if (TempNodeIndices.FindValue(TempNodeParentName, TempNodeParentIndex))
					{
						NewBone.Parent = TempNodeParentIndex;
					}
				}
			}
			OutBoneIndices.Add(NewBone.Name, NewBone.Index);
		}

		// Read skeleton default transform.
		AssimpReadNodeDefaultTransformRecursion(InScene->mRootNode, TempNodeNames, OutBoneIndices, OutBones);

		// Save node's child relation.
		for (SHORT NodeIndex = 0, NodeNum = static_cast<SHORT>(OutBones.Length()); NodeIndex < NodeNum; NodeIndex++)
		{
			EBoneData& BoneData = OutBones[NodeIndex];
			if ((NodeIndex != BoneData.Index) || (BoneData.Parent < 0) || (BoneData.Parent >= NodeNum))
			{
				continue;
			}
			EBoneData& BoneParentData = OutBones[BoneData.Parent];
			BoneParentData.Children.Add(BoneData.Index);
		}

		return TRUE;
	}
	static BOOL32 GatherAllAnimationChannelDatas(const TMap<EString, const aiAnimation*>& InAnimations, TArray<ESkeletonAnimationClip>& OutSkeletonAnimationClips)
	{
		if (OutSkeletonAnimationClips.Length() > 0u)
		{
			OutSkeletonAnimationClips.Clear();
		}
		BOOL32 Result = TRUE;
		for (auto It = InAnimations.Begin(); It != InAnimations.End(); It++)
		{
			OutSkeletonAnimationClips.Add(ESkeletonAnimationClip(It->first));
			ESkeletonAnimationClip& NewAnimationClip = OutSkeletonAnimationClips[OutSkeletonAnimationClips.Length() - 1u];
			const aiAnimation* Animation = It->second;
			NewAnimationClip.Duration = static_cast<FLOAT>(Animation->mDuration);
			NewAnimationClip.TicksPerSecond = static_cast<FLOAT>(Animation->mTicksPerSecond);
			AssimpGatherSingleAnimationChannelData(Animation, NewAnimationClip);
		}
		return Result;
	}
	template<typename _TMesh>
	static void GatherMeshBounds(_TMesh& InOutMesh)
	{
		EBoundAABB ResultBound(Vector3(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX), Vector3(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX), TRUE);

		for (UINT32 i = 0u; i < EMesh::MeshVertexLayoutPartMaxNum; i++)
		{
			const EVertexData* VertexDataPtr = nullptr;
			if (InOutMesh.GetVertexElement(EVertexLayoutType::MESH_VERTEX, i, VertexDataPtr))
			{
				if (VertexDataPtr)
				{
					const UINT32 VertexElementNum = VertexDataPtr->Stride / sizeof(FLOAT);
					for (UINT32 VertexIndex = 0u; VertexIndex < VertexDataPtr->ElementNum; VertexIndex++)
					{
						Vector3 TempVertex(Vector3::Zero());
						if (VertexElementNum > 0)
						{
							TempVertex.x = VertexDataPtr->Datas[VertexIndex * VertexElementNum + 0];
						}
						if (VertexElementNum > 1)
						{
							TempVertex.y = VertexDataPtr->Datas[VertexIndex * VertexElementNum + 1];
						}
						if (VertexElementNum > 2)
						{
							TempVertex.z = VertexDataPtr->Datas[VertexIndex * VertexElementNum + 2];
						}
						ResultBound.AABBMin = MinVector3(TempVertex, ResultBound.AABBMin);
						ResultBound.AABBMax = MaxVector3(TempVertex, ResultBound.AABBMax);
					}
				}
			}
		}
		
		if ((ResultBound.AABBMax - ResultBound.AABBMin).Length() <= EEngineSettings::ENGINE_BOUND_MINIMUM)
		{
			Vector3 TempOrigin = (ResultBound.AABBMax + ResultBound.AABBMin) * 0.5f;
			Vector3 TempExtent = Vector3(EEngineSettings::ENGINE_BOUND_MINIMUM_HALF, EEngineSettings::ENGINE_BOUND_MINIMUM_HALF, EEngineSettings::ENGINE_BOUND_MINIMUM_HALF);
			ResultBound.AABBMin = TempOrigin - TempExtent;
			ResultBound.AABBMax = TempOrigin + TempExtent;
		}

		InOutMesh.SetBoundAABB(ResultBound.AABBMin, ResultBound.AABBMax);
	}
	void CAssimpManager::Initialize()
	{
		if (_GAssetImporter == nullptr)
		{
			_GAssetImporter = new Assimp::Importer();
		}
	}
	void CAssimpManager::ShutDown()
	{
		if (_GAssetImporter != nullptr)
		{
			_GAssetImporter->FreeScene();
			delete (_GAssetImporter);
			_GAssetImporter = nullptr;
		}
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadStaticMeshFile(const EString& InPath, TArray<EStaticMesh>& OutMeshes, const BOOL32 IsCombineSubmeshes)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutMeshes.Length() > 0u)
		{
			for (UINT32 i = 0u, n = OutMeshes.Length(); i < n; i++)
			{
				OutMeshes[i].ReleaseResource();
			}
			OutMeshes.Clear();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		const aiScene* Scene = AssImporter->ReadFile(
			*InPath,
			aiProcess_CalcTangentSpace |
			aiProcess_JoinIdenticalVertices |
			aiProcess_MakeLeftHanded |
			aiProcess_Triangulate |
			aiProcess_RemoveComponent |
			aiProcess_GenSmoothNormals |
			aiProcess_SplitLargeMeshes |
			/*aiProcess_ImproveCacheLocality |*/
			aiProcess_RemoveRedundantMaterials |
			aiProcess_FixInfacingNormals |
			aiProcess_SortByPType |
			aiProcess_FindInvalidData |
			aiProcess_GenUVCoords |
			aiProcess_OptimizeMeshes |
			aiProcess_OptimizeGraph |
			aiProcess_FlipUVs |
			aiProcess_FlipWindingOrder |
			aiProcess_GenBoundingBoxes);

		// If the import failed, report it
		if (Scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		if (!(Scene->HasMeshes()))
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Scene does not contain meshes
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		// Only access first mesh in scene.
		TArray<const aiMesh*> Meshes;
		{
			TArray<const aiMesh*> TempMeshes; TArray<TArray<RShaderSemanticType>> TempMeshesLayouts; TArray<BOOL32> TempIsSkeletalMesh;
			FindMeshesAndVertexLayouts(Scene, TempMeshes, TempMeshesLayouts, TempIsSkeletalMesh);
			PE_CHECK((ENGINE_ASSET_ERROR), ("Meshes and layouts are not matched."), (TempMeshes.Length() > 0u && TempMeshes.Length() == TempMeshesLayouts.Length() && TempMeshes.Length() == TempIsSkeletalMesh.Length()));
			for (UINT32 i = 0u, n = TempMeshes.Length(); i < n; i++)
			{
				if (TempIsSkeletalMesh[i])
				{
					continue;
				}
				Meshes.Add(TempMeshes[i]);
			}
		}

		if (Meshes.Length() == 0u)
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Scene does not contain static meshes
			AssImporter->FreeScene();
			return Result;
		}

		PE_CHECK((ENGINE_ASSET_ERROR), ("Meshes and layouts are not matched."), (Meshes.Length() > 0u));

		const RShaderSemanticType* EngineLayouts; UINT32 EngineLayoutNum;
		GetEngineDefaultMeshInputLayouts(EngineLayouts, EngineLayoutNum);

		TArray<EStaticMesh> TempOutMeshes;
		if (IsCombineSubmeshes)
		{
			TranslateAssimpMeshToEngineMeshInternal(EngineLayouts, EngineLayoutNum, Meshes, TempOutMeshes);

			EStaticMesh& NewStaticMesh = OutMeshes.Add_Default_GetRef();
			if (TempOutMeshes.Length() > 0u)
			{
				NewStaticMesh.SetMeshName(TempOutMeshes[0].GetMeshName());
			}

			CombineEngineMeshIntoSubmeshes(TempOutMeshes, NewStaticMesh);

			GatherMeshBounds(NewStaticMesh);
		}
		else
		{
			TranslateAssimpMeshToEngineMeshInternal(EngineLayouts, EngineLayoutNum, Meshes, OutMeshes);

			for (UINT32 OutMeshIndex = 0u, OutMeshNum = OutMeshes.Length(); OutMeshIndex < OutMeshNum; OutMeshIndex++)
			{
				GatherMeshBounds(OutMeshes[OutMeshIndex]);
			}
		}

		Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED;
		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadSkeletonFile(const EString& InPath, ESkeleton& OutSkeleton)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutSkeleton.GetBoneCount() > 0u)
		{
			OutSkeleton.ReleaseResource();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		//const aiScene* scene = impoter->ReadFile(
		//	path,
		//	aiProcess_CalcTangentSpace |
		//	aiProcess_JoinIdenticalVertices |
		//	aiProcess_MakeLeftHanded |
		//	aiProcess_Triangulate |
		//	aiProcess_RemoveComponent |
		//	aiProcess_GenSmoothNormals |
		//	aiProcess_SplitLargeMeshes |
		//	/*aiProcess_LimitBoneWeights |*/
		//	aiProcess_RemoveRedundantMaterials |
		//	aiProcess_FixInfacingNormals |
		//	aiProcess_PopulateArmatureData |
		//	aiProcess_SortByPType |
		//	aiProcess_FindInvalidData |
		//	aiProcess_GenUVCoords |
		//	aiProcess_OptimizeMeshes |
		//	aiProcess_FlipUVs |
		//	aiProcess_FlipWindingOrder |
		//	/*aiProcess_SplitByBoneCount |*/
		//	/*aiProcess_Debone |*/
		//	aiProcess_GenBoundingBoxes);
		const aiScene* Scene = AssImporter->ReadFile(*InPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);

		// If the import failed, report it
		if (Scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		TArray<EBoneData> BoneDatas; TMap<EString, SHORT> BoneIndices;
		if (!GatherAllBoneNodeDatas(Scene, BoneDatas, BoneIndices))
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		for (UINT32 i = 0u, n = BoneDatas.Length(); i < n; i++)
		{
			OutSkeleton.AddBoneElement(&(BoneDatas[i]));
		}

		Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED;
		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadSkinnedMeshFile(const EString& InPath, TArray<ESkinnedMesh>& OutMeshes, const BOOL32 IsCombineSubmeshes)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutMeshes.Length() > 0u)
		{
			for (UINT32 i = 0u, n = OutMeshes.Length(); i < n; i++)
			{
				OutMeshes[i].ReleaseResource();
			}
			OutMeshes.Clear();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		//const aiScene* scene = impoter->ReadFile(
		//	path,
		//	aiProcess_CalcTangentSpace |
		//	aiProcess_JoinIdenticalVertices |
		//	aiProcess_MakeLeftHanded |
		//	aiProcess_Triangulate |
		//	aiProcess_RemoveComponent |
		//	aiProcess_GenSmoothNormals |
		//	aiProcess_SplitLargeMeshes |
		//	/*aiProcess_LimitBoneWeights |*/
		//	aiProcess_RemoveRedundantMaterials |
		//	aiProcess_FixInfacingNormals |
		//	aiProcess_PopulateArmatureData |
		//	aiProcess_SortByPType |
		//	aiProcess_FindInvalidData |
		//	aiProcess_GenUVCoords |
		//	aiProcess_OptimizeMeshes |
		//	aiProcess_FlipUVs |
		//	aiProcess_FlipWindingOrder |
		//	/*aiProcess_SplitByBoneCount |*/
		//	/*aiProcess_Debone |*/
		//	aiProcess_GenBoundingBoxes);
		const aiScene* Scene = AssImporter->ReadFile(*InPath, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_ConvertToLeftHanded);

		// If the import failed, report it
		if (Scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		if (!(Scene->HasMeshes()))
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		// Read data structures of all nodes.
		TArray<const aiMesh*> Meshes;
		{
			TArray<const aiMesh*> TempMeshes; TArray<TArray<RShaderSemanticType>> TempMeshesLayouts; TArray<BOOL32> TempIsSkeletalMesh;
			FindMeshesAndVertexLayouts(Scene, TempMeshes, TempMeshesLayouts, TempIsSkeletalMesh);
			PE_CHECK((ENGINE_ASSET_ERROR), ("Meshes and layouts are not matched."), (TempMeshes.Length() > 0u && TempMeshes.Length() == TempMeshesLayouts.Length() && TempMeshes.Length() == TempIsSkeletalMesh.Length()));
			for (UINT32 i = 0u, n = TempMeshes.Length(); i < n; i++)
			{
				if (!(TempIsSkeletalMesh[i]))
				{
					continue;
				}
				Meshes.Add(TempMeshes[i]);
			}
		}

		if (Meshes.Length() == 0u)
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Scene does not contain static meshes
			AssImporter->FreeScene();
			return Result;
		}

		const RShaderSemanticType* EngineLayouts; UINT32 EngineLayoutNum;
		GetEngineDefaultSkeletalMeshInputLayouts(EngineLayouts, EngineLayoutNum);

		TArray<ESkinnedMesh> TempOutMeshes;
		if (IsCombineSubmeshes)
		{
			TranslateAssimpMeshToEngineMeshInternal(EngineLayouts, EngineLayoutNum, Meshes, TempOutMeshes);

			ESkinnedMesh& NewSkinnedMesh = OutMeshes.Add_Default_GetRef();
			if (TempOutMeshes.Length() > 0u)
			{
				NewSkinnedMesh.SetMeshName(TempOutMeshes[0].GetMeshName());
			}
			USHORT EffectBoneNum = 0u;
			for (UINT32 MeshIndex = 0u, MeshNum = TempOutMeshes.Length(); MeshIndex < MeshNum; MeshIndex++)
			{
				ESkinnedMesh& TempSkinnedMesh = TempOutMeshes[MeshIndex];
				EffectBoneNum = EMath::Max(EffectBoneNum, TempSkinnedMesh.GetEffectBoneNum());
			}

			CombineEngineMeshIntoSubmeshes(TempOutMeshes, EffectBoneNum, NewSkinnedMesh);

			GatherMeshBounds(NewSkinnedMesh);
		}
		else
		{
			TranslateAssimpMeshToEngineMeshInternal(EngineLayouts, EngineLayoutNum, Meshes, OutMeshes);

			for (UINT32 OutMeshIndex = 0u, OutMeshNum = OutMeshes.Length(); OutMeshIndex < OutMeshNum; OutMeshIndex++)
			{
				GatherMeshBounds(OutMeshes[OutMeshIndex]);
			}
		}

		Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED;
		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}
	CAssimpManager::CReadFileStateType CAssimpManager::ReadSkeletonAnimationFile(const EString& path, TArray<ESkeletonAnimationClip>& OutSkeletonAnimationClips)
	{
		CReadFileStateType Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_FAILED;

		if (OutSkeletonAnimationClips.Length() > 0u)
		{
			OutSkeletonAnimationClips.Clear();
		}

		Assimp::Importer* AssImporter = _GAssetImporter;
		if (AssImporter == nullptr)
		{
			// TODO Do the error logging (did not create the instance of importer)
			return Result;
		}

		// And have it read the given file with some example postprocessing
		// Usually - if speed is not the most important aspect for you - you'll
		// probably to request more postprocessing than we do in this example.

		// Use SetPropertyInteger to modify config of importer
		//Assimp::Importer::SetProperty###();

		//const aiScene* scene = impoter->ReadFile(
		//	path,
		//	aiProcess_MakeLeftHanded |
		//	aiProcess_RemoveComponent |
		//	aiProcess_RemoveRedundantMaterials |
		//	aiProcess_PopulateArmatureData);
		const aiScene* Scene = AssImporter->ReadFile(*path, aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_MakeLeftHanded);

		// If the import failed, report it
		if (Scene == nullptr)
		{
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		if (!Scene->HasAnimations())
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		// Now we can access the file's contents.
		TMap<EString, const aiAnimation*> SceneAnimations;
		{
			for (UINT32 i = 0u, n = Scene->mNumAnimations; i < n; i++)
			{
				const aiAnimation* Animation = Scene->mAnimations[i];
				if ((!Animation) || (Animation->mNumChannels == 0u))
				{
					continue;
				}
				EString CurrentAnimationName = AssimpTranslateString(Animation->mName);
				EString TempName = CurrentAnimationName;
				BOOL32 ShouldAddNewAnimation = TRUE;
				{
					const aiAnimation* TempValue = nullptr;
					UINT32 SameNameIndex = 0u;
					if (SceneAnimations.Length() > 0u)
					{
						while (SceneAnimations.FindValue(TempName, TempValue))
						{
							if (TempValue == Animation)
							{
								ShouldAddNewAnimation = FALSE;
								break;
							}
							TempName = CurrentAnimationName + ToString(SameNameIndex);
							SameNameIndex += 1u;
						}
					}
				}
				if (ShouldAddNewAnimation)
				{
					SceneAnimations.Add(TempName, Animation);
				}
			}
		}

		if (SceneAnimations.Length() == 0u)
		{
			Result = CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;
			// TODO Do the error logging (importer.GetErrorString())
			AssImporter->FreeScene();
			return Result;
		}

		Result = (GatherAllAnimationChannelDatas(SceneAnimations, OutSkeletonAnimationClips) == TRUE) ? CReadFileStateType::ASSIMP_READ_FILE_STATE_SUCCEED : CReadFileStateType::ASSIMP_READ_FILE_STATE_ERROR;

		// We're done. Everything will be cleaned up by the importer destructor
		AssImporter->FreeScene();
		return Result;
	}

};