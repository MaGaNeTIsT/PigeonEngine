#pragma once

#include <Base/BuiltInLanguage.h>

#ifndef _DEBUG_MODE
#define _DEBUG_MODE 0
#else
#define _DEBUG_MODE 1
#endif
#ifndef _DEVELOP_MODE
#define _DEVELOP_MODE 0
#else
#define _DEVELOP_MODE 1
#endif
#ifndef _TEST_MODE
#define _TEST_MODE 0
#else
#define _TEST_MODE 1
#endif
#ifndef _SHIPPING_MODE
#define _SHIPPING_MODE 0
#else
#define _SHIPPING_MODE 1
#endif
#if (_DEBUG_MODE || _DEVELOP_MODE)
#define _EDITOR_ONLY 1
#else
#define _EDITOR_ONLY 0
#endif

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

#if _EDITOR_ONLY
#include "../../../ThirdParty/imgui/imgui.h"
#endif