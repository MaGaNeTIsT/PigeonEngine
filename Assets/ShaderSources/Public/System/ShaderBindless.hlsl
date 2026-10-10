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
// Register path (default)
//     Texture2D<float4> BaseColorMap : register(t3);      <- from the JSON
//     PE_SAMPLE_TEX2D(BaseColorMap, SamplerMaterial2D, UV)
//         -> BaseColorMap.Sample(SamplerMaterial2D, UV)
//
// Bindless path (PE_SHADER_BINDLESS = 1)
//     The JSON declares a uint per resource inside a constant buffer instead of
//     a register bound object, named <ResourceName>_Idx. The macro resolves the
//     index through ResourceDescriptorHeap.
// ---------------------------------------------------------------------------

#ifndef PE_SHADER_BINDLESS
#define PE_SHADER_BINDLESS					0
#endif	//PE_SHADER_BINDLESS

#if PE_SHADER_BINDLESS

#define PE_BINDLESS_TEX1D_CAST(InIndex)			((Texture1D<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(InIndex)])
#define PE_BINDLESS_TEX2D_CAST(InIndex)			((Texture2D<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(InIndex)])
#define PE_BINDLESS_TEX3D_CAST(InIndex)			((Texture3D<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(InIndex)])
#define PE_BINDLESS_TEXCUBE_CAST(InIndex)		((TextureCube<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(InIndex)])
#define PE_BINDLESS_TEX2DARRAY_CAST(InIndex)	((Texture2DArray<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(InIndex)])
#define PE_BINDLESS_BUFFER_CAST(InIndex)		((StructuredBuffer<float4>)ResourceDescriptorHeap[NonUniformResourceIndex(InIndex)])

// Resolves a resource name to the constant buffer field that holds its heap index.
#define PE_TEXTURE_BINDING(InName)				InName##_Idx
#define PE_BUFFER_BINDING(InName)				InName##_Idx

#define PE_SAMPLE_TEX2D(InName, InSampler, InUV) \
	PE_BINDLESS_TEX2D_CAST(PE_TEXTURE_BINDING(InName)).Sample(InSampler, InUV)
#define PE_SAMPLE_TEX2D_LEVEL(InName, InSampler, InUV, InLod) \
	PE_BINDLESS_TEX2D_CAST(PE_TEXTURE_BINDING(InName)).SampleLevel(InSampler, InUV, InLod)
#define PE_SAMPLE_TEX2D_BIAS(InName, InSampler, InUV, InBias) \
	PE_BINDLESS_TEX2D_CAST(PE_TEXTURE_BINDING(InName)).SampleBias(InSampler, InUV, InBias)
#define PE_SAMPLE_TEX2D_GRAD(InName, InSampler, InUV, InDdx, InDdy) \
	PE_BINDLESS_TEX2D_CAST(PE_TEXTURE_BINDING(InName)).SampleGrad(InSampler, InUV, InDdx, InDdy)
#define PE_LOAD_TEX2D(InName, InCoord) \
	PE_BINDLESS_TEX2D_CAST(PE_TEXTURE_BINDING(InName)).Load(InCoord)

#define PE_SAMPLE_TEX3D(InName, InSampler, InUVW) \
	PE_BINDLESS_TEX3D_CAST(PE_TEXTURE_BINDING(InName)).Sample(InSampler, InUVW)
#define PE_SAMPLE_TEX3D_LEVEL(InName, InSampler, InUVW, InLod) \
	PE_BINDLESS_TEX3D_CAST(PE_TEXTURE_BINDING(InName)).SampleLevel(InSampler, InUVW, InLod)

#define PE_SAMPLE_TEXCUBE(InName, InSampler, InDirection) \
	PE_BINDLESS_TEXCUBE_CAST(PE_TEXTURE_BINDING(InName)).Sample(InSampler, InDirection)
#define PE_SAMPLE_TEXCUBE_LEVEL(InName, InSampler, InDirection, InLod) \
	PE_BINDLESS_TEXCUBE_CAST(PE_TEXTURE_BINDING(InName)).SampleLevel(InSampler, InDirection, InLod)

#define PE_SAMPLE_TEX2DARRAY(InName, InSampler, InUV) \
	PE_BINDLESS_TEX2DARRAY_CAST(PE_TEXTURE_BINDING(InName)).Sample(InSampler, InUV)
#define PE_SAMPLE_TEX2DARRAY_LEVEL(InName, InSampler, InUV, InLod) \
	PE_BINDLESS_TEX2DARRAY_CAST(PE_TEXTURE_BINDING(InName)).SampleLevel(InSampler, InUV, InLod)

// Engine samplers stay statically embedded in the root signature on both paths,
// so the sampler argument is passed through untouched.
#define PE_SAMPLER_BINDING(InName)				InName

#else

// The resource and the sampler are ordinary register bound globals emitted by
// the compiler from the JSON declarations, so the macros forward unchanged.
#define PE_TEXTURE_BINDING(InName)				InName
#define PE_BUFFER_BINDING(InName)				InName
#define PE_SAMPLER_BINDING(InName)				InName

#define PE_SAMPLE_TEX2D(InName, InSampler, InUV) \
	InName.Sample(InSampler, InUV)
#define PE_SAMPLE_TEX2D_LEVEL(InName, InSampler, InUV, InLod) \
	InName.SampleLevel(InSampler, InUV, InLod)
#define PE_SAMPLE_TEX2D_BIAS(InName, InSampler, InUV, InBias) \
	InName.SampleBias(InSampler, InUV, InBias)
#define PE_SAMPLE_TEX2D_GRAD(InName, InSampler, InUV, InDdx, InDdy) \
	InName.SampleGrad(InSampler, InUV, InDdx, InDdy)
#define PE_LOAD_TEX2D(InName, InCoord) \
	InName.Load(InCoord)

#define PE_SAMPLE_TEX3D(InName, InSampler, InUVW) \
	InName.Sample(InSampler, InUVW)
#define PE_SAMPLE_TEX3D_LEVEL(InName, InSampler, InUVW, InLod) \
	InName.SampleLevel(InSampler, InUVW, InLod)

#define PE_SAMPLE_TEXCUBE(InName, InSampler, InDirection) \
	InName.Sample(InSampler, InDirection)
#define PE_SAMPLE_TEXCUBE_LEVEL(InName, InSampler, InDirection, InLod) \
	InName.SampleLevel(InSampler, InDirection, InLod)

#define PE_SAMPLE_TEX2DARRAY(InName, InSampler, InUV) \
	InName.Sample(InSampler, InUV)
#define PE_SAMPLE_TEX2DARRAY_LEVEL(InName, InSampler, InUV, InLod) \
	InName.SampleLevel(InSampler, InUV, InLod)

#endif	//PE_SHADER_BINDLESS
#endif	//SHADER_BINDLESS_HLSL
