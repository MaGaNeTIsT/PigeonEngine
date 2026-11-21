#include <Navigation/NavMeshProfile.h>

#include <Asset/NavMeshAsset/NavMeshAsset.h>
#include <Navigation/NavigationProviderInterface.h>
#include <Reflection/Property.h>

namespace PigeonEngine
{

	EString MakeNavMeshAssetName(const EString& InLevelStem, const EString& InAgentName)
	{
		if (InAgentName.Length() == 0u)
		{
			return InLevelStem + EString(PE_TEXT("_NavMesh"));
		}
		// File name safe form of the agent: a name like "Large Agent" must not split the asset
		// into another directory or lose the suffix.
		EString Sanitized;
		for (UINT32 Index = 0u, Num = InAgentName.Length(); Index < Num; Index++)
		{
			const CHAR Character = InAgentName[Index];
			const BOOL32 bAllowed = (((Character >= 'A') && (Character <= 'Z'))
				|| ((Character >= 'a') && (Character <= 'z'))
				|| ((Character >= '0') && (Character <= '9'))
				|| (Character == '_') || (Character == '-')) ? TRUE : FALSE;
			if (bAllowed)
			{
				const CHAR CharacterBuffer[2] = { Character, '\0' };
				Sanitized += CharacterBuffer;
			}
			else
			{
				Sanitized += EString(PE_TEXT("_"));
			}
		}
		return InLevelStem + EString(PE_TEXT("_")) + Sanitized + EString(PE_TEXT("_NavMesh"));
	}

	BOOL32 WriteNavMeshAsset(ENavigationProviderInterface& InProvider, const ENavMeshBuildSettings& InBuildSettings,
		const ENavigationBuildStats& InStats, const EString& InAssetName,
		EString& OutError, const BOOL32& bInAllowOverwrite)
	{
		OutError = EString();
		if (InAssetName.Length() == 0u)
		{
			OutError = EString(PE_TEXT("The navmesh asset name is empty."));
			return FALSE;
		}
		TArray<UINT8> Blob;
		if (!InProvider.ExportData(Blob))
		{
			OutError = EString(PE_TEXT("The baked navmesh could not be exported."));
			return FALSE;
		}

		FNavMeshAssetSettings Settings;
		Settings.ProviderName = ERecastNavigationProvider::GetProviderName().ToString();
		Settings.BuildSettings = InBuildSettings;
		Settings.Bounds = InProvider.GetBounds();
		Settings.PolygonCount = InStats.PolygonCount;
		Settings.TileCount = InStats.TileCount;

		const ENavMeshAsset* SavedAsset = nullptr;
		if (!ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			InAssetName, Settings, Blob, EString(EBaseSettings::ENGINE_NAV_MESH_PATH), SavedAsset, OutError, bInAllowOverwrite))
		{
			return FALSE;
		}
		return TRUE;
	}

	void SerializeNavMeshBuildSettings(EArchive& InArchive, const CHAR* InName, ENavMeshBuildSettings& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeFloat(PE_TEXT("CellSize"), InOutValue.CellSize);
		InArchive.SerializeFloat(PE_TEXT("CellHeight"), InOutValue.CellHeight);
		InArchive.SerializeFloat(PE_TEXT("AgentRadius"), InOutValue.AgentRadius);
		InArchive.SerializeFloat(PE_TEXT("AgentHeight"), InOutValue.AgentHeight);
		InArchive.SerializeFloat(PE_TEXT("AgentMaxClimb"), InOutValue.AgentMaxClimb);
		InArchive.SerializeFloat(PE_TEXT("AgentMaxSlopeAngle"), InOutValue.AgentMaxSlopeAngle);
		InArchive.SerializeFloat(PE_TEXT("MaxEdgeLength"), InOutValue.MaxEdgeLength);
		InArchive.SerializeFloat(PE_TEXT("MaxSimplificationError"), InOutValue.MaxSimplificationError);
		InArchive.SerializeFloat(PE_TEXT("DetailSampleDistance"), InOutValue.DetailSampleDistance);
		InArchive.SerializeFloat(PE_TEXT("DetailSampleMaxError"), InOutValue.DetailSampleMaxError);
		InArchive.SerializeUInt32(PE_TEXT("MinRegionArea"), InOutValue.MinRegionArea);
		InArchive.SerializeUInt32(PE_TEXT("MergeRegionArea"), InOutValue.MergeRegionArea);
		InArchive.SerializeUInt32(PE_TEXT("MaxVertsPerPoly"), InOutValue.MaxVertsPerPoly);
		InArchive.SerializeUInt32(PE_TEXT("TileSizeCells"), InOutValue.TileSizeCells);
		InArchive.SerializeUInt32(PE_TEXT("MaxLayersPerTile"), InOutValue.MaxLayersPerTile);
		InArchive.SerializeUInt32(PE_TEXT("MaxCrowdAgents"), InOutValue.MaxCrowdAgents);
		InArchive.SerializeBool(PE_TEXT("FlipTriangleWinding"), InOutValue.bFlipTriangleWinding);
		InArchive.EndObject();
	}

	// Authored off-mesh connections, kept as one indexed array of objects so a level file stays
	// readable and an older file without the field simply loads no links.
	void SerializeNavigationLinks(EArchive& InArchive, const CHAR* InName, TArray<ENavigationLink>& InOutLinks)
	{
		UINT32 Count = InOutLinks.Num<UINT32>();
		InArchive.BeginArray(InName, Count);
		if (!InArchive.IsSaving())
		{
			InOutLinks.Empty();
		}
		for (UINT32 Index = 0u; Index < Count; Index++)
		{
			const EString IndexKey = EString::FromInt(Index);
			ENavigationLink Link;
			if (InArchive.IsSaving())
			{
				Link = InOutLinks[Index];
			}
			InArchive.BeginObject(*IndexKey);
			SerializeValue(InArchive, PE_TEXT("Start"), Link.Start);
			SerializeValue(InArchive, PE_TEXT("End"), Link.End);
			InArchive.SerializeFloat(PE_TEXT("Radius"), Link.Radius);
			InArchive.SerializeBool(PE_TEXT("Directional"), Link.bDirectional);
			INT32 AreaValue = static_cast<INT32>(Link.Area);
			SerializeEnumValue(InArchive, PE_TEXT("Area"), AreaValue, GetClassHashCode<ENavigationAreaFlagType>());
			Link.Area = static_cast<ENavigationAreaFlagType>(AreaValue);
			InArchive.EndObject();
			if (!InArchive.IsSaving())
			{
				InOutLinks.Add(Link);
			}
		}
		InArchive.EndArray();
	}

	void SerializeNavMeshProfile(EArchive& InArchive, const CHAR* InName, FNavMeshProfile& InOutProfile)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeString(PE_TEXT("Name"), InOutProfile.Name);
		SerializeNavMeshBuildSettings(InArchive, PE_TEXT("BuildSettings"), InOutProfile.BuildSettings);
		InArchive.SerializeString(PE_TEXT("NavMeshAsset"), InOutProfile.NavMeshAsset);
		SerializeNavigationLinks(InArchive, PE_TEXT("NavLinks"), InOutProfile.NavLinks);
		InArchive.EndObject();
	}

	namespace
	{
		void RegisterBuildSettingsStruct()
		{
			const size_t StructHash = GetClassHashCode<ENavMeshBuildSettings>();
			const EString StructName(PE_TEXT("ENavMeshBuildSettings"));
			const UINT32 EditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE);
			AddStructProperty(StructHash, StructName, PE_TEXT("CellSize"), &ENavMeshBuildSettings::CellSize, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("CellHeight"), &ENavMeshBuildSettings::CellHeight, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("AgentRadius"), &ENavMeshBuildSettings::AgentRadius, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("AgentHeight"), &ENavMeshBuildSettings::AgentHeight, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("AgentMaxClimb"), &ENavMeshBuildSettings::AgentMaxClimb, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("AgentMaxSlopeAngle"), &ENavMeshBuildSettings::AgentMaxSlopeAngle, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("MaxEdgeLength"), &ENavMeshBuildSettings::MaxEdgeLength, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("MaxSimplificationError"), &ENavMeshBuildSettings::MaxSimplificationError, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("DetailSampleDistance"), &ENavMeshBuildSettings::DetailSampleDistance, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("DetailSampleMaxError"), &ENavMeshBuildSettings::DetailSampleMaxError, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("MinRegionArea"), &ENavMeshBuildSettings::MinRegionArea, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("MergeRegionArea"), &ENavMeshBuildSettings::MergeRegionArea, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("MaxVertsPerPoly"), &ENavMeshBuildSettings::MaxVertsPerPoly, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("TileSizeCells"), &ENavMeshBuildSettings::TileSizeCells, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("MaxLayersPerTile"), &ENavMeshBuildSettings::MaxLayersPerTile, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("MaxCrowdAgents"), &ENavMeshBuildSettings::MaxCrowdAgents, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("FlipTriangleWinding"), &ENavMeshBuildSettings::bFlipTriangleWinding, EditFlags);
		}

		void RegisterNavigationLinkStruct()
		{
			const size_t StructHash = GetClassHashCode<ENavigationLink>();
			const EString StructName(PE_TEXT("ENavigationLink"));
			const UINT32 EditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE);
			AddStructProperty(StructHash, StructName, PE_TEXT("Start"), &ENavigationLink::Start, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("End"), &ENavigationLink::End, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("Radius"), &ENavigationLink::Radius, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("Directional"), &ENavigationLink::bDirectional, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("Area"), &ENavigationLink::Area, EditFlags);
		}

		void RegisterProfileStruct()
		{
			const size_t StructHash = GetClassHashCode<FNavMeshProfile>();
			const EString StructName(PE_TEXT("FNavMeshProfile"));
			const UINT32 EditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE);
			AddStructProperty(StructHash, StructName, PE_TEXT("Name"), &FNavMeshProfile::Name, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("BuildSettings"), &FNavMeshProfile::BuildSettings, EditFlags);
			AddStructAssetProperty(StructHash, StructName, PE_TEXT("NavMeshAsset"), &FNavMeshProfile::NavMeshAsset, EPropertyAssetType::NAV_MESH, EditFlags);
			AddStructProperty(StructHash, StructName, PE_TEXT("NavLinks"), &FNavMeshProfile::NavLinks, EditFlags);
		}
	}

	void RegisterNavMeshProfileTypes()
	{
		// One shot: registering the same descriptor twice appends duplicate fields and shifts the
		// positional value tree, so every caller shares this guard.
		static BOOL32 bRegistered = FALSE;
		if (bRegistered != FALSE)
		{
			return;
		}
		bRegistered = TRUE;
		RegisterBuildSettingsStruct();
		RegisterNavigationLinkStruct();
		RegisterProfileStruct();
	}

}  // namespace PigeonEngine
