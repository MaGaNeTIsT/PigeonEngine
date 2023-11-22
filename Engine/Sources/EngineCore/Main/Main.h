#pragma once

#include <Base/BuiltInLanguage.h>

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <stdio.h>
#include <ctime>
#include <assert.h>
#include <io.h>
#include <tchar.h>
#include <dwmapi.h>

#include <Base/DataStructure/BuiltInType.h>
#include <BaseConfig.h>

#if _EDITOR_ONLY
#include "../../../ThirdParty/imgui/imgui.h"
#endif