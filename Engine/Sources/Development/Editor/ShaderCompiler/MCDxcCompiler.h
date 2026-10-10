#pragma once

#include <string>
#include <vector>
#include <cstdint>


struct IDxcUtils;
struct IDxcCompiler3;
struct IDxcIncludeHandler;

namespace MC
{

	struct MCDxcCbVariable
	{
		std::string			Name		= "";
		uint32_t			ByteOffset	= 0;		// Offset of the variable relative to the start of its constant buffer.
		uint32_t			ContentBytes	= 0;		// Content bytes of the variable: internal padding included, trailing slot padding excluded.
		uint32_t			ArrayStride	= 0;		// 0 when the variable is not an array.
	};

	struct MCDxcCbLayout
	{
		std::string			Name		= "";
		uint32_t			ByteSize	= 0;		// Shader visible size of the constant buffer.
		std::vector<MCDxcCbVariable>	Variables	= {};
	};

	enum class MCDxcBindingKind : uint8_t
	{
		Binding_CBuffer,
		Binding_Srv,			// Textures and Resources share one srv slot range.
		Binding_Uav,			// RWTextures and RWResources share one uav slot range.
		Binding_Sampler,
	};

	struct MCDxcBinding
	{
		std::string			Name		= "";		// Empty when the compiler stripped resource names (optimized builds).
		uint32_t			BindPoint	= 0;
		MCDxcBindingKind		Kind		= MCDxcBindingKind::Binding_Srv;
	};

	struct MCDxcResult
	{
		std::vector<uint8_t>		DxilBytes;
		std::vector<uint8_t>		PdbBytes;
		std::vector<MCDxcCbLayout>	CBuffers;			// Constant buffer layout reported by the compiler reflection.
		std::vector<MCDxcBinding>	Bindings;			// Resources the compiled shader actually binds.
		std::string			Errors;
		bool				Success		= false;
	};

	struct MCCompileParams
	{
		using TDefine = std::pair<std::string, std::string>;

		std::string						HlslSource;					// Complete HLSL string (already assembled by JSON layer / file read)
		std::string						SourceName;					// For error messages, e.g. "M_DefaultLit.ps"
		std::string						Profile;					// "vs_6_0", "ps_6_0", "cs_6_0", "ps_5_0", etc.
		std::string						EntryName;					// "main", "VSMain", etc.
		std::vector<std::string>		IncludeDirs;				// Unified include search paths (engine shaders, templates, material source, etc.)
		std::vector<TDefine>			Defines;
		bool							bDebug		= false;		// Emit debug info (Zi + Qembed_debug) + PDB
		bool							bDx11		= false;		// true -> D3DCompile (DXBC), false -> DXC (DXIL)
	};

	class MCDxcCompiler final
	{
	public:
		MCDxcCompiler() noexcept;
		~MCDxcCompiler() noexcept;

		bool Init(std::string& OutError) noexcept;
		void Shutdown() noexcept;

		// Pure compiler: takes an already-assembled HLSL string + compile parameters, returns compiled bytes.
		// No template processing or JSON awareness - the caller is responsible for assembling the source.
		bool Compile(const MCCompileParams& InParams, MCDxcResult& OutResult) noexcept;

	private:
		IDxcUtils*				DXCUtils				= nullptr;
		IDxcCompiler3*			DXCCompiler				= nullptr;
		IDxcIncludeHandler*		DXCIncludeHandler		= nullptr;

		bool CompileDxc(const MCCompileParams& InParams, MCDxcResult& OutResult) noexcept;
		bool CompileD3D(const MCCompileParams& InParams, MCDxcResult& OutResult) noexcept;
	};

}	// namespace MC
