#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{
	class CJsonObject;
	class PWorld;
	class PActor;

	/*
	* Level file v1: a FormatVersion plus a flat actor list. Each actor entry carries its
	* stable class name, class version, parent actor file id (0 = attached to the world
	* root), its reflected properties and its components; each component entry carries its
	* class name, class version, root flag, intra-actor parent index and its reflected
	* properties.
	*
	* The container is written through EArchive arrays, so a binary archive backend can
	* serialize a level without touching this code. Asset references serialize as their
	* asset name (meshes for now); object references will use FObjectHandle in a later
	* format revision.
	*/
	namespace ELevelSerializer
	{
		PE_CONSTEXPR_STATIC UINT32 LevelFormatVersion = 1u;

		// Saves exactly the given actors; parent ids are only written for parents that are
		// part of the same set.
		BOOL32 SaveActors(const TArray<PActor*>& InActors, CJsonObject& OutDocument);
		// Saves every actor in the world (the world root actor itself is not an entry; its
		// children are saved with parent id 0).
		BOOL32 SaveWorld(const PWorld& InWorld, CJsonObject& OutDocument);
		BOOL32 SaveActorsToFile(const TArray<PActor*>& InActors, const EString& InPath);

		/*
		* Creates the document's actors in InWorld. Every created actor is appended to
		* OutCreatedActors when provided. Top-level entries are attached to the world root
		* actor and child entries to the loaded actor their file id names; to undo a load,
		* destroy the entries whose parent is not another actor of this same load - child
		* actors go with their parent.
		*/
		BOOL32 LoadActors(PWorld& InWorld, const CJsonObject& InDocument, TArray<PActor*>* OutCreatedActors = nullptr);
		BOOL32 LoadActorsFromFile(PWorld& InWorld, const EString& InPath, TArray<PActor*>* OutCreatedActors = nullptr);

	}

}
