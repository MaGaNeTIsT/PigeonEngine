#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{

	class ESkeletonAsset;

	/*
	* Edits the ordered slot groups stored on a skeleton. The order is the montage blend
	* order: group 0 is the base layer and each later group is blended on top.
	*/
	class ESkeletonSlotGroupEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenSkeletonAsset(const EString& InAssetFullPath);

	private:
		BOOL32 ValidateSlotGroups(EString& OutError)const;

	private:
		BOOL8						m_ShowEditor = FALSE;
		const ESkeletonAsset*		m_Asset = nullptr;
		EString						m_OpenedAssetFullPath;
		EString						m_LastError;
	};

}

#endif
