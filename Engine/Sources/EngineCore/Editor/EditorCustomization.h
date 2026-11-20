#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <Base/Delegate/Function.h>
#include <Base/RTTI/RTTIManager.h>

namespace PigeonEngine
{
	class PActor;
	class PActorComponent;
	class PWorld;

	/*
	* Per-class details editors. Runtime classes carry no ImGui code; the editor module
	* registers one draw function per class hash, and the details host composes every
	* registration found along the object's inheritance chain, base-first. That is the
	* same shape the old GenerateComponentDetail virtual chain had, where each override
	* called its base before drawing its own extras.
	*
	* The reflection grid (EPropertyEditor::DrawProperties) is drawn once by the host
	* before the chain runs, so a registration draws only its class's own extras. A
	* class with no registration anywhere on its chain simply shows the grid.
	*/
	namespace EEditorCustomization
	{
		using FDrawActorDetails = TFunction<void(PActor*, PWorld*)>;
		using FDrawComponentDetails = TFunction<void(PActorComponent*, PWorld*)>;

		void RegisterActorCustomEditor(const size_t& InClassHash, const FDrawActorDetails& InDraw);
		void RegisterComponentCustomEditor(const size_t& InClassHash, const FDrawComponentDetails& InDraw);

		template <typename TClass>
		void RegisterActorCustomEditor(const FDrawActorDetails& InDraw)
		{
			RegisterActorCustomEditor(GetClassHashCode<TClass>(), InDraw);
		}

		template <typename TClass>
		void RegisterComponentCustomEditor(const FDrawComponentDetails& InDraw)
		{
			RegisterComponentCustomEditor(GetClassHashCode<TClass>(), InDraw);
		}

		// Draws the reflection grid once, then every editor registered along the object's
		// inheritance chain, base-first. Returns FALSE when the object is null.
		BOOL32 DrawActorDetails(PActor* InActor, PWorld* InWorld);
		BOOL32 DrawComponentDetails(PActorComponent* InComponent, PWorld* InWorld);

		// How many editors the chain for this class would run. Used by the framework sanity
		// check to cover registration and inheritance without an ImGui frame.
		PE_NODISCARD UINT32 CountActorEditorsForClass(const size_t& InClassHash);
		PE_NODISCARD UINT32 CountComponentEditorsForClass(const size_t& InClassHash);
	}
}

#endif
