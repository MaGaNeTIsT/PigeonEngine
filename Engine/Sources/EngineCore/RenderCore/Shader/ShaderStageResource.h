#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>
#include <RenderCommon.h>
#include <RHI/IRRHIResource.h>
#include <RHI/RHITypes.h>
#include <ShaderAsset/ShaderReflection.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - thin render layer for one compiled stage.
	*
	* The shader module itself never talks to the RHI: it keeps the bytecode and the
	* reflection. This is the one place where a stage is turned into a device object,
	* and where the compiler's semantic strings become render types, so the reflection
	* layer stays free of render types and this layer stays free of asset concerns.
	*
	* The resource owns the shader object the RHI handed back. On D3D11 the back-end
	* returns a concrete shader resource and on D3D12 a back-end shader; both are
	* IRRHIShader, so the wrapper stores the interface and never has to branch by
	* back-end.
	*/

	// Compiler semantic string ("POSITION", "TEXCOORD", ...) to render semantic.
	PE_NODISCARD extern BOOL32	TranslateShaderInputSemantic(const EName& InSemanticName, RShaderSemanticType& OutSemantic);
	// Asset stage to RHI stage. The compile side carries stages a pipeline cannot
	// consume (geometry / hull / domain), so this can fail and the caller has to decide
	// what an unsupported stage means for it.
	PE_NODISCARD extern BOOL32	TranslateShaderStageToRHI(const EShaderStageType InStage, ERHIShaderStage& OutStage);
	// Reflection input layout to the render description the RHI / PSO wants. Fails on
	// layouts RInputLayoutDesc cannot express (matrix attributes).
	PE_NODISCARD extern BOOL32	BuildShaderInputLayouts(const TArray<EShaderReflectionInputLayout>& InReflectionLayouts, TArray<RInputLayoutDesc>& OutLayouts);

	class RShaderStageResource : public RRenderResourceInterface
	{
	public:
		RShaderStageResource();
		virtual ~RShaderStageResource() override;
	public:
		virtual BOOL8 IsRenderResourceValid() const noexcept override;
		virtual BOOL8 InitRenderResource() noexcept override;
		virtual void ReleaseRenderResource() noexcept override;
	public:
		BOOL32	CreateRenderResource(const void* InBytecode, const ULONG InByteSize
			, const ERHIShaderStage InRHIShaderStage, const TArray<EShaderReflectionInputLayout>& InReflectionLayouts);
		PE_NODISCARD IRRHIShader*	GetRHIShader() const { return RHIShader; }
		PE_NODISCARD ERHIShaderStage	GetRHIShaderStage() const { return RHIShaderStage; }
		// Kept so downstream code can feed the same description into PSO creation.
		PE_NODISCARD const TArray<RInputLayoutDesc>&	GetInputLayouts() const { return InputLayouts; }
	private:
		IRRHIShader*				RHIShader;
		ERHIShaderStage				RHIShaderStage;
		TArray<RInputLayoutDesc>	InputLayouts;

		CLASS_REMOVE_COPY_BODY(RShaderStageResource)

	};

} // namespace PigeonEngine
