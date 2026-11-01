#pragma once

#if _EDITOR_ONLY

#include "../../../Sources/EngineCore/Main/Main.h"
#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/BaseType.h>
#include <Base/DataStructure/Text/String.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/ObjectBase.h>
#include <RenderCommon.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <AnimationAsset/AnimationAsset.h>

namespace PigeonEngine
{
	/*
	* The material one source mesh was authored with, resolved against the scene's own material
	* list. A mesh asset carries geometry and a name only, so a caller that has to know which
	* material - and which texture files - an object came from would otherwise have to parse the
	* source file a second time. The spellings here are the source's: a texture path is relative to
	* the source file, extension included, exactly as the file wrote it.
	*/
	struct FImportedMeshMaterial
	{
	public:
		EString	MaterialName;
		// The material's own diffuse colour, which is what a material without a diffuse map has to
		// be drawn with - its map is not there to replace it.
		Color3	DiffuseColor	= Color3(1.f, 1.f, 1.f);
		// Empty when the source assigns the mesh no texture of that kind.
		EString	DiffuseMapPath;
		EString	NormalMapPath;
		// The source's coverage map (an obj's map_d). A non-empty value is what marks the material
		// as one that wants the Mask surface mode; the file itself is not required to exist for
		// that marker to be useful, because the cutout may already live in the albedo's alpha.
		EString	MaskMapPath;
	};

	class CAssimpManager : public EManagerBase
	{
	public:
		enum CReadFileStateType : UINT8
		{
			ASSIMP_READ_FILE_STATE_SUCCEED,
			ASSIMP_READ_FILE_STATE_FAILED,
			ASSIMP_READ_FILE_STATE_ERROR
		};
	public:
		virtual void Initialize()override;
		virtual void ShutDown()override;
	public:
		/*
		* InIsOptimizeMeshes decides whether assimp may merge meshes. Merging is right for a
		* runtime asset - it collapses meshes that share a material into one - and wrong for an
		* importer that wants one object per authored object, which is why it is the caller's
		* choice rather than a property of this function.
		*
		* OutMaterials, when given, receives one entry per source mesh, in the same order as the
		* meshes: with InIsCombineSubmeshes the combined mesh carries one submesh per source mesh,
		* so the entries also line up with its submeshes.
		*/
		CReadFileStateType ReadStaticMeshFile(const EString& InPath, TArray<EStaticMesh>& OutMeshes, const BOOL32 InIsCombineSubmeshes
			, const BOOL32 InIsOptimizeMeshes = TRUE, TArray<FImportedMeshMaterial>* OutMaterials = nullptr);
		CReadFileStateType ReadSkeletonFile(const EString& InPath, ESkeleton& OutSkeleton);
		CReadFileStateType ReadSkinnedMeshFile(const EString& InPath, TArray<ESkinnedMesh>& OutMeshes, const BOOL32 IsCombineSubmeshes);
		CReadFileStateType ReadSkeletonAnimationFile(const EString& path, TArray<ESkeletonAnimationClip>& OutSkeletonAnimationClips);

		CLASS_MANAGER_SINGLETON_BODY(CAssimpManager)

	};
};

#endif
