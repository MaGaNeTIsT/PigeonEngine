#ifndef SHADER_BINDLESS_HLSL
#define SHADER_BINDLESS_HLSL

// ---------------------------------------------------------------------------
// Bindless / register binding indirection for texture and buffer access.
//
// Every sample in the shader library, in a domain and in a material goes
// through PE_SAMPLE_* / PE_LOAD_*. That single choke point is what lets the
// bindless path be switched on later without touching a single call site: only
// this file changes.
//
// Register path
//     Texture2D<float4> BaseColorMap : register(t3);      <- from the JSON
//     PE_SAMPLE_TEX2D(BaseColorMap, SamplerMaterial2D, UV)
//         -> BaseColorMap.Sample(SamplerMaterial2D, UV)
//
// Bindless path
//     uint BaseColorMap_BindlessIdx;                      <- generated into the constant
//                                                            buffer the JSON names with
//                                                            'BindConstantBuffer'
//     PE_SAMPLE_TEX2D(BaseColorMap, SamplerMaterial2D, UV)
//         -> PE_BINDLESS_TEX2D_CAST(BaseColorMap_BindlessIdx).Sample(...)
//
// The two are the same code here. The compiler writes one binding name per declared resource
// (PE_BINDING_<Name>) that resolves to either form, so every macro below is written once and
// compiles for both. The index suffix lives in EngineShaderConfig.h and is never spelled here.
// ---------------------------------------------------------------------------

// The compiler writes PE_BINDLESS_COMPILED as 1 when this compilation emitted heap indexed bindings
// and 0 when it did not, and the library has to know, because the access functions that carry a heap
// index cannot be parsed by every compiler the engine targets: a 5.0 build compiles through FXC,
// which has no templates and no heap objects, so that path is written out without them. It is the
// only bindless switch on this side: whether a resource is index driven is the compiler's answer,
// and the runtime reads the same answer back from the reflection instead of a second macro.
#ifndef PE_BINDLESS_COMPILED
#define PE_BINDLESS_COMPILED	0
#endif	//PE_BINDLESS_COMPILED

#if PE_BINDLESS_COMPILED

// The cast macros are the contract between this file and the binding names the compiler writes
// below. The element type is the first argument because the JSON declared it: the compiler passes
// the type it read from the declaration, so the heap object is typed exactly like the register
// bound one and no guess is made here. ResourceDescriptorHeap and SamplerDescriptorHeap exist from
// shader model 6.6 on, so a cast is only ever expanded on the bindless path: a target that cannot
// index the heap never names one, and these definitions stay dormant text.
//
// The index is not wrapped in NonUniformResourceIndex: the engine gives each resource one index per
// material, so the index is uniform across a draw, and the wrapper is what the compiler's own
// internal error trips over. A resource that ever needs per invocation indices would have to bring
// the wrapper back after that compiler defect is gone.
#define PE_BINDLESS_TEX1D_CAST(InElementType, InIndex)				((Texture1D<InElementType>)ResourceDescriptorHeap[InIndex])
#define PE_BINDLESS_TEX2D_CAST(InElementType, InIndex)				((Texture2D<InElementType>)ResourceDescriptorHeap[InIndex])
#define PE_BINDLESS_TEX3D_CAST(InElementType, InIndex)				((Texture3D<InElementType>)ResourceDescriptorHeap[InIndex])
#define PE_BINDLESS_TEXCUBE_CAST(InElementType, InIndex)			((TextureCube<InElementType>)ResourceDescriptorHeap[InIndex])
#define PE_BINDLESS_TEX2DARRAY_CAST(InElementType, InIndex)			((Texture2DArray<InElementType>)ResourceDescriptorHeap[InIndex])
#define PE_BINDLESS_BUFFER_CAST(InElementType, InIndex)				((Buffer<InElementType>)ResourceDescriptorHeap[InIndex])
#define PE_BINDLESS_STRUCTURED_BUFFER_CAST(InElementType, InIndex)	((StructuredBuffer<InElementType>)ResourceDescriptorHeap[InIndex])
#define PE_BINDLESS_BYTE_ADDRESS_BUFFER_CAST(InIndex)				((ByteAddressBuffer)ResourceDescriptorHeap[InIndex])
// SamplerDescriptorHeap takes the same shape as the resource heap: cast, and let the result land in
// a typed operand before anything is called on it.
#define PE_BINDLESS_SAMPLER_CAST(InIndex)		((SamplerState)SamplerDescriptorHeap[InIndex])

// One binding name per declared resource, written by the compiler: PE_BINDING_<ResourceName>
// resolves to the heap index cast to the right object type when this build made the resource index
// driven, and to the resource itself when it did not. The two levels are what lets the name be
// pasted and then expanded, and that is what keeps every call below identical on both paths.
#define PE_BINDING_OF_IMPL(InName)		PE_BINDING_##InName
#define PE_BINDING_OF(InName)			PE_BINDING_OF_IMPL(InName)

#define PE_TEXTURE_BINDING(InName)		PE_BINDING_OF(InName)
#define PE_BUFFER_BINDING(InName)		PE_BINDING_OF(InName)
#define PE_SAMPLER_BINDING(InName)		PE_BINDING_OF(InName)

// Every access goes through one small function, because the object a binding name yields has to
// land in a typed operand before a member is called on it: this compiler rejects a member call
// written straight onto a heap index expression, and a parameter is the shape it accepts. The
// functions are generic in the element type, so whatever type the JSON declared is the type that
// arrives here, and the same functions serve both paths - a register bound object is passed in
// exactly like a heap index is.
template<typename TElement>
TElement	PEBindlessSampleTex2D(Texture2D<TElement> InTexture, SamplerState InSampler, float2 InUV)
{
	return InTexture.Sample(InSampler, InUV);
}
template<typename TElement>
TElement	PEBindlessSampleTex2DLevel(Texture2D<TElement> InTexture, SamplerState InSampler, float2 InUV, float InLod)
{
	return InTexture.SampleLevel(InSampler, InUV, InLod);
}
template<typename TElement>
TElement	PEBindlessSampleTex2DBias(Texture2D<TElement> InTexture, SamplerState InSampler, float2 InUV, float InBias)
{
	return InTexture.SampleBias(InSampler, InUV, InBias);
}
template<typename TElement>
TElement	PEBindlessSampleTex2DGrad(Texture2D<TElement> InTexture, SamplerState InSampler, float2 InUV, float2 InDdx, float2 InDdy)
{
	return InTexture.SampleGrad(InSampler, InUV, InDdx, InDdy);
}
template<typename TElement>
TElement	PEBindlessLoadTex2D(Texture2D<TElement> InTexture, int3 InCoord)
{
	return InTexture.Load(InCoord);
}
template<typename TElement>
TElement	PEBindlessSampleTex3D(Texture3D<TElement> InTexture, SamplerState InSampler, float3 InUVW)
{
	return InTexture.Sample(InSampler, InUVW);
}
template<typename TElement>
TElement	PEBindlessSampleTex3DLevel(Texture3D<TElement> InTexture, SamplerState InSampler, float3 InUVW, float InLod)
{
	return InTexture.SampleLevel(InSampler, InUVW, InLod);
}
template<typename TElement>
TElement	PEBindlessSampleTexCube(TextureCube<TElement> InTexture, SamplerState InSampler, float3 InDirection)
{
	return InTexture.Sample(InSampler, InDirection);
}
template<typename TElement>
TElement	PEBindlessSampleTexCubeLevel(TextureCube<TElement> InTexture, SamplerState InSampler, float3 InDirection, float InLod)
{
	return InTexture.SampleLevel(InSampler, InDirection, InLod);
}
template<typename TElement>
TElement	PEBindlessSampleTex2DArray(Texture2DArray<TElement> InTexture, SamplerState InSampler, float3 InUV)
{
	return InTexture.Sample(InSampler, InUV);
}
template<typename TElement>
TElement	PEBindlessSampleTex2DArrayLevel(Texture2DArray<TElement> InTexture, SamplerState InSampler, float3 InUV, float InLod)
{
	return InTexture.SampleLevel(InSampler, InUV, InLod);
}

#define PE_SAMPLE_TEX2D(InName, InSampler, InUV) \
	PEBindlessSampleTex2D(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUV)
#define PE_SAMPLE_TEX2D_LEVEL(InName, InSampler, InUV, InLod) \
	PEBindlessSampleTex2DLevel(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUV, InLod)
#define PE_SAMPLE_TEX2D_BIAS(InName, InSampler, InUV, InBias) \
	PEBindlessSampleTex2DBias(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUV, InBias)
#define PE_SAMPLE_TEX2D_GRAD(InName, InSampler, InUV, InDdx, InDdy) \
	PEBindlessSampleTex2DGrad(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUV, InDdx, InDdy)
#define PE_LOAD_TEX2D(InName, InCoord) \
	PEBindlessLoadTex2D(PE_TEXTURE_BINDING(InName), InCoord)
#define PE_SAMPLE_TEX3D(InName, InSampler, InUVW) \
	PEBindlessSampleTex3D(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUVW)
#define PE_SAMPLE_TEX3D_LEVEL(InName, InSampler, InUVW, InLod) \
	PEBindlessSampleTex3DLevel(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUVW, InLod)
#define PE_SAMPLE_TEXCUBE(InName, InSampler, InDirection) \
	PEBindlessSampleTexCube(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InDirection)
#define PE_SAMPLE_TEXCUBE_LEVEL(InName, InSampler, InDirection, InLod) \
	PEBindlessSampleTexCubeLevel(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InDirection, InLod)
#define PE_SAMPLE_TEX2DARRAY(InName, InSampler, InUV) \
	PEBindlessSampleTex2DArray(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUV)
#define PE_SAMPLE_TEX2DARRAY_LEVEL(InName, InSampler, InUV, InLod) \
	PEBindlessSampleTex2DArrayLevel(PE_TEXTURE_BINDING(InName), PE_SAMPLER_BINDING(InSampler), InUV, InLod)

#else

// Register path. The declarations are ordinary register bound globals the compiler emitted from the
// JSON, so every access is written straight out: no heap object, no cast, no helper, and therefore
// no assumption about the element type either. The binding names resolve to the declarations
// themselves, which is what the compiler wrote for this build.
#define PE_TEXTURE_BINDING(InName)		InName
#define PE_BUFFER_BINDING(InName)		InName
#define PE_SAMPLER_BINDING(InName)		InName

#define PE_SAMPLE_TEX2D(InName, InSampler, InUV)					InName.Sample(InSampler, InUV)
#define PE_SAMPLE_TEX2D_LEVEL(InName, InSampler, InUV, InLod)		InName.SampleLevel(InSampler, InUV, InLod)
#define PE_SAMPLE_TEX2D_BIAS(InName, InSampler, InUV, InBias)		InName.SampleBias(InSampler, InUV, InBias)
#define PE_SAMPLE_TEX2D_GRAD(InName, InSampler, InUV, InDdx, InDdy)	InName.SampleGrad(InSampler, InUV, InDdx, InDdy)
#define PE_LOAD_TEX2D(InName, InCoord)								InName.Load(InCoord)
#define PE_SAMPLE_TEX3D(InName, InSampler, InUVW)					InName.Sample(InSampler, InUVW)
#define PE_SAMPLE_TEX3D_LEVEL(InName, InSampler, InUVW, InLod)		InName.SampleLevel(InSampler, InUVW, InLod)
#define PE_SAMPLE_TEXCUBE(InName, InSampler, InDirection)			InName.Sample(InSampler, InDirection)
#define PE_SAMPLE_TEXCUBE_LEVEL(InName, InSampler, InDirection, InLod)	InName.SampleLevel(InSampler, InDirection, InLod)
#define PE_SAMPLE_TEX2DARRAY(InName, InSampler, InUV)				InName.Sample(InSampler, InUV)
#define PE_SAMPLE_TEX2DARRAY_LEVEL(InName, InSampler, InUV, InLod)	InName.SampleLevel(InSampler, InUV, InLod)

#endif	//PE_BINDLESS_COMPILED

#endif	//SHADER_BINDLESS_HLSL
