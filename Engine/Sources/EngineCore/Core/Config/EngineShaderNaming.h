#pragma once
// Engine shader naming conventions.
// This file is intentionally free of any includes so that standalone tool
// projects (e.g. MaterialCompiler) can include it without pulling in engine
// headers.  Add Engine/Sources/EngineCore/Core to the tool's include paths
// and use: #include <Config/EngineShaderNaming.h>

// ── Shader binary suffixes ────────────────────────────────────────────────
// Must stay in sync with EEngineSettings::ENGINE_IMPORT_*_SHADER_NAME_TYPE.
#define MC_SHADER_VS_SUFFIX  "_PEVS"
#define MC_SHADER_PS_SUFFIX  "_PEPS"
#define MC_SHADER_CS_SUFFIX  "_PECS"

// ── Material Compiler build-time paths ───────────────────────────────────
// Relative to the engine working directory.  Override in project-specific
// configuration if your layout differs.
#define MC_MATERIAL_OUTPUT_DIR          "./Build/Temp/MaterialShader/"
#define MC_MATERIAL_SOURCE_DIR          "./Assets/MaterialSources/"
#define MC_MATERIAL_SHADER_INCLUDE_DIR  "./Engine/Shaders/Include/"
