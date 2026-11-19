#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{
	class CJsonObject;
	class PWorld;
	class PActor;

	/*
	* Level header: the world settings a saved level restores independently of its actors.
	* Kept separate from PWorld so a caller can read a file without instantiating anything.
	*/
	struct FLevelHeader
	{
		EString		LevelName;
		Vector3		GravityDirection = -Vector3::YVector();
		FLOAT		GravityStrength = 981.f;
		BOOL32		bUseUpVector = TRUE;
		Vector3		UpVector = Vector3::YVector();
	};

	/*
	* Level file v2: a FormatVersion, an optional Level header and a flat actor list. Each
	* actor entry carries its stable class name, class version, editor debug name (editor
	* builds), parent actor file id (0 = attached to the world root) and its components;
	* each component entry carries its class name, class version, debug name, root flag,
	* intra-actor parent index and its reflected properties.
	*
	* Object references are stored as file-local "actorFileId:componentIndex" strings and
	* resolved after the whole actor list exists. Asset references are stored by asset name.
	*
	* The container is written through EArchive, so a binary archive backend can serialize
	* a level without touching this code. Format v1 (no Level header, no names) still loads:
	* missing fields keep their defaults.
	*/
	namespace ELevelSerializer
	{
		PE_CONSTEXPR_STATIC UINT32 LevelFormatVersion = 2u;

		// Saves exactly the given actors; parent ids are only written for parents that are
		// part of the same set.
		BOOL32 SaveActors(const TArray<PActor*>& InActors, CJsonObject& OutDocument);
		// Saves every actor whose ShouldSerializeToLevel() is true, plus the world header.
		BOOL32 SaveWorld(const PWorld& InWorld, const EString& InLevelName, CJsonObject& OutDocument);
		BOOL32 SaveActorsToFile(const TArray<PActor*>& InActors, const EString& InPath);
		BOOL32 SaveWorldToFile(const PWorld& InWorld, const EString& InLevelName, const EString& InPath);

		BOOL32 ReadLevelHeader(const CJsonObject& InDocument, FLevelHeader& OutHeader);

		/*
		* Creates the document's actors in InWorld. Every created actor is appended to
		* OutCreatedActors when provided. Top-level entries are attached to the world root
		* actor and child entries to the loaded actor their file id names; to undo a load,
		* destroy the entries whose parent is not another actor of this same load - child
		* actors go with their parent.
		*/
		BOOL32 LoadActors(PWorld& InWorld, const CJsonObject& InDocument, TArray<PActor*>* OutCreatedActors = nullptr);
		BOOL32 LoadWorld(PWorld& InWorld, const CJsonObject& InDocument, FLevelHeader* OutHeader = nullptr, TArray<PActor*>* OutCreatedActors = nullptr);
		BOOL32 LoadActorsFromFile(PWorld& InWorld, const EString& InPath, TArray<PActor*>* OutCreatedActors = nullptr);
		BOOL32 LoadWorldFromFile(PWorld& InWorld, const EString& InPath, FLevelHeader* OutHeader = nullptr, TArray<PActor*>* OutCreatedActors = nullptr);

	}
}
