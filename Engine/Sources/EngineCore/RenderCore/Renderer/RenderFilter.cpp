#include "RenderFilter.h"

#include <PhysicsConfig/PhysicsConfig.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/ViewProxy.h>
#include <Renderer/ScenePass.h>

namespace PigeonEngine
{

	/*
	* Collection rules. The order is the contract: a rule that rejects stops the evaluation,
	* and the cheap mask tests come before the ones that inspect a proxy.
	*/

	static BOOL32 RuleViewCategoryMask(const FRenderFilterContext& InContext)
	{
		const UINT64 ViewMask = InContext.View->GetPrimitiveCategoryMask();
		if (ViewMask == 0ull)
		{
			// A view that states no mask accepts every category. This is the default for a
			// camera view, so the common case costs one comparison here.
			return TRUE;
		}
		return (((ViewMask & InContext.Primitive->GetPrimitiveCategoryMask()) != 0ull) ? TRUE : FALSE);
	}

	static BOOL32 RuleShadowCaster(const FRenderFilterContext& InContext)
	{
		if (InContext.View->GetViewKind() != EViewKindType::VIEW_KIND_SHADOW)
		{
			return TRUE;
		}
		return InContext.Primitive->IsSceneProxyCastShadow();
	}

	static BOOL32 RuleDebugDrawEnabled(const FRenderFilterContext& InContext)
	{
		const UINT64 CategoryMask = InContext.Primitive->GetPrimitiveCategoryMask();
		const UINT64 DebugMask = (ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_DEBUG
			| ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_COLLISION);
		if ((CategoryMask & DebugMask) == 0ull)
		{
			return TRUE;
		}
		if ((InContext.View->GetFilterFlags() & EViewFilterFlagType::VIEW_FILTER_FLAG_HIDE_DEBUG) != 0ull)
		{
			return FALSE;
		}
#if ENGINE_EDITOR_ONLY
		return ((FPhysicsCommonSettings::GetSingleton()->bDrawCollision != FALSE) ? TRUE : FALSE);
#else
		return TRUE;
#endif
	}

	static BOOL32 RuleEditorOnly(const FRenderFilterContext& InContext)
	{
		if ((InContext.Primitive->GetPrimitiveCategoryMask() & ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_EDITOR_ONLY) == 0ull)
		{
			return TRUE;
		}
		return (((InContext.View->GetFilterFlags() & EViewFilterFlagType::VIEW_FILTER_FLAG_EDITOR) != 0ull) ? TRUE : FALSE);
	}

	/*
	* Draw rules. Pass x View membership is checked by the pass object first; this table
	* covers the parts that need the primitive and the global policy only.
	*/

	static BOOL32 RulePassCategoryMask(const FRenderFilterContext& InContext)
	{
		const UINT64 PassMask = InContext.PassDesc->PrimitiveCategoryMask;
		if (PassMask == 0ull)
		{
			return TRUE;
		}
		return (((PassMask & InContext.Primitive->GetPrimitiveCategoryMask()) != 0ull) ? TRUE : FALSE);
	}

	static const FRenderFilterRule BuiltinRules[] =
	{
		{ ERenderFilterScopeType::RENDER_FILTER_SCOPE_COLLECT,	&RuleViewCategoryMask },
		{ ERenderFilterScopeType::RENDER_FILTER_SCOPE_COLLECT,	&RuleShadowCaster },
		{ ERenderFilterScopeType::RENDER_FILTER_SCOPE_COLLECT,	&RuleDebugDrawEnabled },
		{ ERenderFilterScopeType::RENDER_FILTER_SCOPE_COLLECT,	&RuleEditorOnly },
		{ ERenderFilterScopeType::RENDER_FILTER_SCOPE_DRAW,		&RulePassCategoryMask },
		{ ERenderFilterScopeType::RENDER_FILTER_SCOPE_DRAW,		&RuleDebugDrawEnabled },
		{ ERenderFilterScopeType::RENDER_FILTER_SCOPE_DRAW,		&RuleEditorOnly }
	};

	BOOL32 FRenderFilterRegistry::ShouldCollect(const FRenderFilterContext& InContext)
	{
		if ((!InContext.View) || (!InContext.Primitive))
		{
			return FALSE;
		}
		// The object's own decision runs first: a primitive that knows it does not belong to
		// this view is not asked any policy question afterwards.
		if (InContext.Primitive->ShouldCollectInView(*InContext.View) == FALSE)
		{
			return FALSE;
		}
		if (InContext.View->ShouldIncludePrimitive(*InContext.Primitive) == FALSE)
		{
			return FALSE;
		}
		return EvaluateRules(ERenderFilterScopeType::RENDER_FILTER_SCOPE_COLLECT, InContext);
	}

	BOOL32 FRenderFilterRegistry::ShouldDraw(const FRenderFilterContext& InContext)
	{
		if ((!InContext.View) || (!InContext.Primitive) || (!InContext.PassDesc))
		{
			return FALSE;
		}
		if (InContext.View->IsPassVisible(*InContext.PassDesc) == FALSE)
		{
			return FALSE;
		}
		if (InContext.Primitive->ShouldDrawInPass(*InContext.PassDesc) == FALSE)
		{
			return FALSE;
		}
		return EvaluateRules(ERenderFilterScopeType::RENDER_FILTER_SCOPE_DRAW, InContext);
	}

	BOOL32 FRenderFilterRegistry::EvaluateRules(const ERenderFilterScopeType InScope, const FRenderFilterContext& InContext)
	{
		for (UINT32 RuleIndex = 0u, RuleNum = static_cast<UINT32>(sizeof(BuiltinRules) / sizeof(BuiltinRules[0])); RuleIndex < RuleNum; RuleIndex++)
		{
			const FRenderFilterRule& Rule = BuiltinRules[RuleIndex];
			if (Rule.Scope != InScope)
			{
				continue;
			}
			if ((Rule.Predicate) && (Rule.Predicate(InContext) == FALSE))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

}	// namespace PigeonEngine
