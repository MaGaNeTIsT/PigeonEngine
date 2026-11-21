#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>

namespace PigeonEngine
{
	class PWorld;

	// Editor-startup lifecycle checks for the object/component framework. Runs once
	// after the world is initialized (from EMainManager::EditorInit), exercises every
	// destroy path that used to be unsafe, and reports one PASS/FAIL log line per
	// scenario. Permanent diagnostics, not a temporary test: it is the regression net
	// for later framework changes, the same role the demo scene plays for rendering.
	namespace EFrameworkSanityCheck
	{
		void RunAllChecks(PWorld* InWorld);
	}
}

#endif
