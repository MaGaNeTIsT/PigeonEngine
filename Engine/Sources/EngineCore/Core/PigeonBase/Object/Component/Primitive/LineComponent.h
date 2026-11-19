#pragma once
#include <CoreMinimal.h>
#include "PrimitiveComponent.h"
namespace PigeonEngine
{
	class PLineComponent : public PPrimitiveComponent
	{
		CLASS_VIRTUAL_NOCOPY_BODY(PLineComponent)
	public:


	public:
		PE_NODISCARD const TArray<Vector3>&	GetPoints() const { return Points; }
		void								SetPoints(const TArray<Vector3>& InPoints);

	protected:
		TArray<Vector3> Points;

		// Render proxy functions START
	public:
		virtual void	CreateRenderState() override;
		virtual void	DestroyRenderState() override;
		virtual void	SendUpdateRenderState() override;
	protected:
		virtual void    MarkRenderTransformAsDirty() override;
		virtual void    MarkRenderStateAsDirty() override;
		virtual void    CleanMarkRenderStateDirty() override;
		// Render proxy functions END
	};
}
