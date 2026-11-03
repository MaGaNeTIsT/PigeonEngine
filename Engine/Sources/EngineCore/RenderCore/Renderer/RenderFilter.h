#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	class RViewProxy;
	class RPrimitiveSceneProxy;
	struct FRenderPassDesc;

	/*
	* What a primitive is, for filtering only. A primitive states one or more of these at
	* construction; a view and a pass state which of them they consume. The bits are not a
	* render queue: two primitives in the same category can still be drawn by different passes.
	*/
	enum ERenderPrimitiveCategoryType : UINT64
	{
		RENDER_PRIMITIVE_CATEGORY_STATIC		= 1ull << 0u,
		RENDER_PRIMITIVE_CATEGORY_SKELETAL		= 1ull << 1u,
		RENDER_PRIMITIVE_CATEGORY_GRASS			= 1ull << 2u,
		RENDER_PRIMITIVE_CATEGORY_WATER			= 1ull << 3u,
		RENDER_PRIMITIVE_CATEGORY_DEBUG			= 1ull << 4u,
		RENDER_PRIMITIVE_CATEGORY_COLLISION		= 1ull << 5u,
		RENDER_PRIMITIVE_CATEGORY_EDITOR_ONLY	= 1ull << 6u,
		RENDER_PRIMITIVE_CATEGORY_TRANSPARENT	= 1ull << 7u
	};

	/*
	* What a view is. The kind decides the default collection rules: a shadow view collects
	* only casters, a reflection view may hide editor only geometry, and a camera view collects
	* everything its own filter mask allows.
	*/
	enum EViewKindType : UINT8
	{
		VIEW_KIND_CAMERA = 0,
		VIEW_KIND_SHADOW,
		VIEW_KIND_REFLECTION,
		VIEW_KIND_COUNT
	};

	enum EViewFilterFlagType : UINT64
	{
		VIEW_FILTER_FLAG_EDITOR			= 1ull << 0u,
		VIEW_FILTER_FLAG_HIDE_DEBUG		= 1ull << 1u
	};

	enum ERenderFilterScopeType : UINT8
	{
		RENDER_FILTER_SCOPE_COLLECT	= 0,
		RENDER_FILTER_SCOPE_DRAW	= 1
	};

	/*
	* Everything one filter decision may read. View, pass and primitive are the three inputs
	* the rules combine; the frame index is carried so a rule that caches can tell frames apart.
	*
	* The context is read only. The filter never mutates a proxy, a pass or a view.
	*/
	struct FRenderFilterContext
	{
		const RViewProxy*			View		= nullptr;
		const FRenderPassDesc*		PassDesc	= nullptr;
		const RPrimitiveSceneProxy*	Primitive	= nullptr;
		UINT64						FrameIndex	= 0ull;
	};

	using FRenderFilterRuleFunction = BOOL32(*)(const FRenderFilterContext& InContext);

	struct FRenderFilterRule
	{
		ERenderFilterScopeType		Scope		= ERenderFilterScopeType::RENDER_FILTER_SCOPE_COLLECT;
		FRenderFilterRuleFunction	Predicate	= nullptr;
	};

	/*
	* The one place view/primitive/pass filtering rules live.
	*
	* Two decisions, evaluated in a fixed order:
	*   ShouldCollect - may this primitive enter this view's primitive list at all.
	*   ShouldDraw    - may this pass consume this primitive from that view.
	*
	* The built-in rules are a table in RenderFilter.cpp. A new engine wide rule is one
	* function and one row there; an object specific exception is a virtual on the primitive,
	* the view or the pass. No rule is spread over the call sites.
	*/
	class FRenderFilterRegistry final
	{
	public:
		// View x Primitive. The caller runs the frustum test afterwards; this answers only
		// the category and policy questions.
		PE_NODISCARD static BOOL32	ShouldCollect(const FRenderFilterContext& InContext);
		// Pass x View x Primitive. The pass object calls this after its own ShouldUseView.
		PE_NODISCARD static BOOL32	ShouldDraw(const FRenderFilterContext& InContext);
	private:
		PE_NODISCARD static BOOL32	EvaluateRules(ERenderFilterScopeType InScope, const FRenderFilterContext& InContext);
	};

}	// namespace PigeonEngine
