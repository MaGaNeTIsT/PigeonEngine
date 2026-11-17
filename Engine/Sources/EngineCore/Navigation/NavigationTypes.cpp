#include <Navigation/NavigationTypes.h>

namespace PigeonEngine
{

	BOOL32 ENavigationPath::IsValid() const
	{
		return (Status != ENavigationPathStatusType::NAVIGATION_PATH_INVALID);
	}

	UINT32 ENavigationPath::Num() const
	{
		return Points.Num<UINT32>();
	}

	void ENavigationPath::Reset()
	{
		Points.Empty();
		PointTypes.Empty();
		Status = ENavigationPathStatusType::NAVIGATION_PATH_INVALID;
	}

}  // namespace PigeonEngine
