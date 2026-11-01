#pragma once
// Engine shader naming conventions.
// This file is intentionally free of any includes so that standalone tool
// projects (e.g. MaterialCompiler) can include it without pulling in engine
// headers.  Add Engine/Sources/EngineCore/Core to the tool's include paths
// and use: #include <Config/EngineShaderConfig.h>

// -- Shader binary suffixes ------------------------------------------------
// The token only, no separator: a name is composed as <Name>_<Token>, so whoever
// concatenates adds the underscore. It used to carry one, which is what made the
// manifest and compile description names come out as <Name>__<Token> while the
// binary and reflection names came out as <Name>_<Variant>_<Token>.
// Must stay in sync with EEngineSettings::ENGINE_IMPORT_*_SHADER_NAME_TYPE.
#define MC_SHADER_VS_SUFFIX		"PEVS"
#define MC_SHADER_PS_SUFFIX		"PEPS"
#define MC_SHADER_CS_SUFFIX		"PECS"

// -- Bindless index naming ------------------------------------------------
// The descriptor heap index of a resource that declares 'PreferBindless' is
// declared as <Name>_<Token>, the same composition rule the binary suffixes
// above follow: the token carries no separator. The compiler stitches it into
// the generated shader header, which is why the shader library never spells the
// name, and the engine uses the same token to find the field it writes the heap
// index into. It lives in this header rather than in a render side one because
// this is the one header the engine and the standalone shader compiler share.
#define MC_SHADER_BINDLESS_INDEX_SUFFIX	"BindlessIdx"

// -- Material Compiler build-time paths -----------------------------------
// Relative to the engine working directory.  Override in project-specific
// configuration if your layout differs.
#define MC_MATERIAL_OUTPUT_DIR			"./Build/Temp/MaterialShader/"
#define MC_MATERIAL_SOURCE_DIR			"./Assets/ShaderSources/"
#define MC_MATERIAL_SHADER_INCLUDE_DIR	"./Assets/ShaderSources/"

// -- Constant buffer limits -----------------------------------------------
// Upper bound of a single shader constant buffer, checked by the shader
// compiler against the size reported by the compiler reflection.  The
// platform limit is 64 KB (D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * 16
// bytes, same for D3D11); lower it to enforce a tighter project budget.
#define MC_MATERIAL_CB_MAX_BYTES		(64 * 1024)

// -- Root constant limits -------------------------------------------------
// Upper bound of a constant buffer marked 'RootConstant' in the shader json
// (root 32 bit constants pushed by the engine).  Checked by the shader
// compiler against the size reported by the compiler reflection; at most one
// such buffer may exist per assembled shader.
#define MC_ROOT_CONSTANT_CB_MAX_BYTES	(16)
