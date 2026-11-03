#include "ViewPrimitiveList.h"
#include <RenderProxy/PrimitiveSceneProxy.h>

namespace PigeonEngine
{

	void RViewPrimitiveBounds::Reset()
	{
		Bounds		= EBoundAABB(Vector3::Zero(), Vector3::Zero(), FALSE);
		bComplete	= TRUE;
	}

	void RViewPrimitiveBounds::Add(const Vector3& InMin, const Vector3& InMax)
	{
		Bounds += EBoundAABB(InMin, InMax, TRUE);
	}

	void RViewPrimitiveBounds::MarkUnknown()
	{
		// The primitive is collected, but its geometry cannot be bounded. The union no longer
		// describes everything the list holds, and a fit built from it could clip the primitive.
		bComplete = FALSE;
	}

	void RViewPrimitiveList::Reset()
	{
		// SetNum(0) keeps the allocation: a view's primitive count is roughly stable from frame
		// to frame, so the storage the previous frame grew is the one this frame wants.
		Items.SetNum(0);
	}

	void RViewPrimitiveList::Add(RPrimitiveSceneProxy* InPrimitive, const EShaderInstance* InShaderInstance)
	{
		if (!InPrimitive)
		{
			return;
		}
		RViewPrimitiveItem& Item = Items.AddDefaultGetRef();
		Item.Primitive		= InPrimitive;
		Item.Identity		= InPrimitive->GetUniqueID();
		Item.ShaderInstance	= InShaderInstance;
	}

	UINT32 RViewPrimitiveList::Num() const
	{
		return Items.Num<UINT32>();
	}

	BOOL32 RViewPrimitiveList::IsEmpty() const
	{
		return ((Items.Num<UINT32>() == 0u) ? TRUE : FALSE);
	}

	TArray<RViewPrimitiveItem>& RViewPrimitiveList::GetItems()
	{
		return Items;
	}

	const TArray<RViewPrimitiveItem>& RViewPrimitiveList::GetItems() const
	{
		return Items;
	}

	RViewPrimitiveItem& RViewPrimitiveList::operator[](const UINT32 InIndex)
	{
		return Items[InIndex];
	}

	const RViewPrimitiveItem& RViewPrimitiveList::operator[](const UINT32 InIndex) const
	{
		return Items[InIndex];
	}

}	// namespace PigeonEngine
