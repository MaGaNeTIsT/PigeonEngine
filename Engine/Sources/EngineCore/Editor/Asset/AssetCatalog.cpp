#include <Editor/Asset/AssetCatalog.h>

#if ENGINE_EDITOR_ONLY

#include <Editor/Asset/AssetTypeUtil.h>

#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <IO/FileHelper.h>

namespace PigeonEngine
{
	namespace
	{
		EString NormalizePath(const EString& InPath)
		{
			EString Result = InPath.Replace("\\", "/");
			while ((Result.Length() > 1u) && (Result[Result.LastIndex()] == '/'))
			{
				Result = Result.Left(Result.Length() - 1u);
			}
			return Result;
		}

		BOOL32 CompareFolderNode(const TSharedPtr<FAssetFolderNode>& InLeft
			, const TSharedPtr<FAssetFolderNode>& InRight)
		{
			return (InLeft->GetSearchName() < InRight->GetSearchName()) ? TRUE : FALSE;
		}

		BOOL32 CompareFileInfo(const TSharedPtr<FAssetFileInfo>& InLeft
			, const TSharedPtr<FAssetFileInfo>& InRight)
		{
			return (InLeft->GetSearchName() < InRight->GetSearchName()) ? TRUE : FALSE;
		}

		TSharedPtr<FAssetFolderNode> FindFolderInNode(const TSharedPtr<FAssetFolderNode>& InNode
			, const EString& InNormalizedPath)
		{
			if (!InNode)
			{
				return nullptr;
			}
			if (NormalizePath(InNode->GetPath()) == InNormalizedPath)
			{
				return InNode;
			}
			const TArray<TSharedPtr<FAssetFolderNode>>& Children = InNode->GetChildrenFolder();
			for (INT32 Index = 0, ChildNum = Children.Num<INT32>(); Index < ChildNum; Index++)
			{
				if (TSharedPtr<FAssetFolderNode> Found = FindFolderInNode(Children[Index], InNormalizedPath))
				{
					return Found;
				}
			}
			return nullptr;
		}
	}

	FAssetFileInfo::FAssetFileInfo(const EString& InPath)
	{
		Path = NormalizePath(InPath);
		DisplayName = EPath::GetFileNameWithoutExtension(Path);
		SearchName = DisplayName.ToLower();
		Type = EAssetTypeUtil::ReadTypeFromFile(Path);
	}

	const EString& FAssetFileInfo::GetPath() const
	{
		return Path;
	}

	const EString& FAssetFileInfo::GetDisplayName() const
	{
		return DisplayName;
	}

	const EString& FAssetFileInfo::GetSearchName() const
	{
		return SearchName;
	}

	EAssetType FAssetFileInfo::GetType() const
	{
		return Type;
	}

	FAssetFolderNode::FAssetFolderNode(const EString& InPath)
	{
		Path = NormalizePath(InPath);
		SearchName = GetDisplayName().ToLower();
	}

	void FAssetFolderNode::SetChildren(const TArray<EString>& InFolderPaths, const TArray<EString>& InFilePaths)
	{
		ChildrenFolder.Empty();
		ChildrenFolder.Reserve(InFolderPaths.Num<UINT32>());
		for (INT32 Index = 0, PathNum = InFolderPaths.Num<INT32>(); Index < PathNum; Index++)
		{
			ChildrenFolder.Add(EMemory::MakeShared<FAssetFolderNode>(InFolderPaths[Index]));
		}
		ChildrenFolder.Sort(&CompareFolderNode);

		ChildrenFile.Empty();
		ChildrenFile.Reserve(InFilePaths.Num<UINT32>());
		for (INT32 Index = 0, PathNum = InFilePaths.Num<INT32>(); Index < PathNum; Index++)
		{
			// Only PAssets belong in the browser grid; ScanDirectory returns every file in
			// the folder, including source images and import sidecars.
			if (InFilePaths[Index].ToLower().Contains(".passet") == FALSE)
			{
				continue;
			}
			ChildrenFile.Add(EMemory::MakeShared<FAssetFileInfo>(InFilePaths[Index]));
		}
		ChildrenFile.Sort(&CompareFileInfo);
	}

	const TArray<TSharedPtr<FAssetFolderNode>>& FAssetFolderNode::GetChildrenFolder() const
	{
		return ChildrenFolder;
	}

	const TArray<TSharedPtr<FAssetFileInfo>>& FAssetFolderNode::GetChildrenFile() const
	{
		return ChildrenFile;
	}

	const EString& FAssetFolderNode::GetPath() const
	{
		return Path;
	}

	const EString& FAssetFolderNode::GetSearchName() const
	{
		return SearchName;
	}

	EString FAssetFolderNode::GetDisplayName() const
	{
		const INT32 SlashIndex = Path.RightFind("/");
		if ((SlashIndex >= 0) && (static_cast<UINT32>(SlashIndex) + 1u < Path.Length()))
		{
			return Path.Substring(static_cast<UINT32>(SlashIndex) + 1u, Path.Length() - static_cast<UINT32>(SlashIndex) - 1u);
		}
		return Path;
	}

	void FAssetCatalog::Initialize()
	{
		Roots.Empty();
		bRefreshRequested = FALSE;
		Refresh();
	}

	void FAssetCatalog::ShutDown()
	{
		Roots.Empty();
		bRefreshRequested = FALSE;
	}

	void FAssetCatalog::RequestRefresh()
	{
		bRefreshRequested = TRUE;
	}

	BOOL32 FAssetCatalog::ConsumeRefreshRequest()
	{
		const BOOL32 bRequested = bRefreshRequested;
		bRefreshRequested = FALSE;
		return bRequested;
	}

	void FAssetCatalog::Refresh()
	{
		// Rebuild every root. The panel keeps the current folder by path, so the new nodes
		// can replace the old ones without invalidating its state. A caller that refreshed
		// immediately (a rename or a move) also clears any request queued for next frame.
		bRefreshRequested = FALSE;
		Roots.Empty();

		struct FRootDesc
		{
			const CHAR*	Label;
			const CHAR*	Path;
		};
		const FRootDesc RootDescs[] =
		{
			// Committed source content first: it is what the editor authors. Engine content
			// is generated under Build/Temp, and the project root may be unset.
			{ "Source", EBaseSettings::ENGINE_ASSET_DIRECTORY },
			{ "Engine", EBaseSettings::ENGINE_CONTENT_PATH },
			{ "Project", EEngineSettings::PROJECT_CONTENT_PATH },
		};

		for (UINT32 RootIndex = 0u; RootIndex < static_cast<UINT32>(sizeof(RootDescs) / sizeof(RootDescs[0])); RootIndex++)
		{
			const EString RootPath(RootDescs[RootIndex].Path);
			if (RootPath.Length() == 0u)
			{
				continue;
			}
			const TSharedPtr<FAssetFolderNode> RootNode = EMemory::MakeShared<FAssetFolderNode>(RootPath);
			TArray<EString> ChildFolders;
			TArray<EString> ChildFiles;
			if (!EFileHelper::ScanDirectory(RootNode->GetPath(), ChildFolders, ChildFiles, TRUE))
			{
				// A root that does not exist (the project path is "UNDEFINED" until a
				// project template sets it) is skipped instead of showing an empty section.
				continue;
			}
			RootNode->SetChildren(ChildFolders, ChildFiles);
			ScanFolder(RootNode);

			FAssetRoot Root;
			Root.Label = EString(RootDescs[RootIndex].Label);
			Root.Node = RootNode;
			Roots.Add(Root);
		}
	}

	const TArray<FAssetRoot>& FAssetCatalog::GetRoots() const
	{
		return Roots;
	}

	TSharedPtr<FAssetFolderNode> FAssetCatalog::FindFolderByPath(const EString& InPath) const
	{
		if (InPath.Length() == 0u)
		{
			return nullptr;
		}
		const EString NormalizedPath = NormalizePath(InPath);
		for (INT32 RootIndex = 0, RootNum = Roots.Num<INT32>(); RootIndex < RootNum; RootIndex++)
		{
			if (TSharedPtr<FAssetFolderNode> Found = FindFolderInNode(Roots[RootIndex].Node, NormalizedPath))
			{
				return Found;
			}
		}
		return nullptr;
	}

	TSharedPtr<FAssetFolderNode> FAssetCatalog::GetDefaultFolder() const
	{
		for (INT32 RootIndex = 0, RootNum = Roots.Num<INT32>(); RootIndex < RootNum; RootIndex++)
		{
			if (Roots[RootIndex].Node)
			{
				return Roots[RootIndex].Node;
			}
		}
		return nullptr;
	}

	void FAssetCatalog::ScanFolder(const TSharedPtr<FAssetFolderNode>& InNode)
	{
		if (!InNode)
		{
			return;
		}
		const TArray<TSharedPtr<FAssetFolderNode>>& Children = InNode->GetChildrenFolder();
		for (INT32 Index = 0, ChildNum = Children.Num<INT32>(); Index < ChildNum; Index++)
		{
			TArray<EString> ChildFolders;
			TArray<EString> ChildFiles;
			if (!EFileHelper::ScanDirectory(Children[Index]->GetPath(), ChildFolders, ChildFiles, TRUE))
			{
				continue;
			}
			Children[Index]->SetChildren(ChildFolders, ChildFiles);
			ScanFolder(Children[Index]);
		}
	}
}

#endif
