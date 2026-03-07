@echo off

setlocal enabledelayedexpansion

set EngineBuildFolder=%~dp0Build

set DecompressLauncher=%~dp0ExtraTools\7z\x64\7za.exe

set AssimpLibFolder=%~dp0Engine\ThirdParty\assimp\lib
set AssimpLibDebugZip=%~dp0Engine\ThirdParty\assimp\lib\Debug.zip
set AssimpLibDebugFolder=%~dp0Engine\ThirdParty\assimp\lib\Debug
set AssimpLibReleaseZip=%~dp0Engine\ThirdParty\assimp\lib\Release.zip
set AssimpLibReleaseFolder=%~dp0Engine\ThirdParty\assimp\lib\Release

if exist "%AssimpLibDebugFolder%" (echo ASSIMP Libraries Debug Folder Exist...)
if not exist "%AssimpLibDebugFolder%" (
	if not exist "%AssimpLibDebugZip%" (echo ASSIMP Libraries Debug.zip Do Not Exist...)
	if exist "%AssimpLibDebugZip%" (
		echo Decompress ASSIMP Libraries Debug...Start
		%DecompressLauncher% x "%AssimpLibDebugZip%" -o"%AssimpLibFolder%"
		echo Decompress ASSIMP Libraries Debug...End
	)
)
if exist "%AssimpLibReleaseFolder%" (echo ASSIMP Libraries Release Folder Exist...)
if not exist "%AssimpLibReleaseFolder%" (
	if not exist "%AssimpLibReleaseZip%" (echo ASSIMP Libraries Release.zip Do Not Exist...)
	if exist "%AssimpLibReleaseZip%" (
		echo Decompress ASSIMP Libraries Release...Start
		%DecompressLauncher% x "%AssimpLibReleaseZip%" -o"%AssimpLibFolder%"
		echo Decompress ASSIMP Libraries Release...End
	)
)

if not exist "%EngineBuildFolder%" (echo Do Not Exist Old Build Folder...)
if exist "%EngineBuildFolder%" (
	echo Delete Old Build Folder...Start
	rd /s /q "%EngineBuildFolder%"
	echo Delete Old Build Folder...End
)

echo Build Pigeon Engine...Start
cmake -B "%EngineBuildFolder%" -DENABLED_WARNING_AS_ERROR=ON -DPLATFORM_WINDOWS=ON -DENABLED_HLSL_DEBUG=ON
echo Build Pigeon Engine...End

pause