#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl/client.h>
#include <stdio.h>
#include <assert.h>
#include <io.h>
#include <strstream>
#include <string>
#include <vector>
#include <list>
#include <map>
using namespace std;

#include <d3d11.h>
#include <DirectXMath.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")
using namespace DirectX;

//#include <fbxsdk.h>
//#pragma comment (lib, "libfbxsdk-mt.lib")
//#pragma comment (lib, "libxml2-mt.lib")
//#pragma comment (lib, "zlib-mt.lib")

#include "../Headers/Base/CTimer.h"


#define ENGINE_SCREEN_WIDTH								(800)
#define ENGINE_SCREEN_HEIGHT							(450)

#define ENGINE_CONSTANT_BUFFER_PER_FRAME_START_SLOT		(0u)
#define ENGINE_CONSTANT_BUFFER_PER_DRAW_START_SLOT		(1u)

#define ENGINE_TEXTURE2D_DEFAULT_WHITE					("./Assets/EngineTextures/Default/EngineDefaultWhite.tga")
#define ENGINE_TEXTURE2D_DEFAULT_BLACK					("./Assets/EngineTextures/Default/EngineDefaultBlack.tga")
#define ENGINE_TEXTURE2D_DEFAULT_RED					("./Assets/EngineTextures/Default/EngineDefaultRed.tga")
#define ENGINE_TEXTURE2D_DEFAULT_GREEN					("./Assets/EngineTextures/Default/EngineDefaultGreen.tga")
#define ENGINE_TEXTURE2D_DEFAULT_BLUE					("./Assets/EngineTextures/Default/EngineDefaultBlue.tga")
#define ENGINE_TEXTURE2D_DEFAULT_BUMP					("./Assets/EngineTextures/Default/EngineDefaultBump.tga")
#define ENGINE_TEXTURE2D_DEFAULT_PROPERTY				("./Assets/EngineTextures/Default/EngineDefaultProperty.tga")

#define ENGINE_SHADER_NONE								("CustomEngine/Shader/None")
#define ENGINE_SHADER_EMPTY_PS							("./HLSLCSO/EngineEmptyPS.cso")
#define ENGINE_SHADER_GBUFFER_WRITE_PS					("./HLSLCSO/EngineGBufferWritePS.cso")
#define ENGINE_SHADER_GBUFFER_RESOLVE_PS				("./HLSLCSO/EngineGBufferResolvePS.cso")
#define ENGINE_SHADER_POST_EFFECT_EXPOSURE_PS			("./HLSLCSO/EnginePostEffectExposurePS.cso")
#define ENGINE_SHADER_SCREEN_POLYGON_2D_VS				("./HLSLCSO/EngineScreenPolygon2DVS.cso")
#define ENGINE_SHADER_SCREEN_POLYGON_2D_PS				("./HLSLCSO/EngineScreenPolygon2DPS.cso")

#define ENGINE_MESH_CUBE_NAME							("CustomEngine/Mesh/CubeMesh")
#define ENGINE_MESH_POLYGON_NAME						("CustomEngine/Mesh/PolygonMesh")
#define ENGINE_MESH_POLYGON_2D_NAME						("CustomEngine/Mesh/Polygon2DMesh")

#define ENGINE_CAMERA_FOV								(60.f)
#define ENGINE_CAMERA_NEAR								(0.1f)
#define ENGINE_CAMERA_FAR								(1000.f)
#define ENGINE_CAMERA_MOVE_SPEED						(20.f)
#define ENGINE_CAMERA_LOOK_SPEED						(270.f)


HWND			GetMainWindowHandle();
CTimer*			GetMainWindowTimer();