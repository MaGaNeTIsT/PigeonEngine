#ifndef VERTEX_FACTORY_TYPES_HLSL
#define VERTEX_FACTORY_TYPES_HLSL

// ---------------------------------------------------------------------------
// Vertex factory identities and the VF_HAS_* presence flags.
//
// This is control point K3 of the design: adding a vertex factory means
// appending a VF_TYPE value here, never renumbering an existing one. VF_TYPE
// ends up in the 16 bit VertexFactoryType field of the variant key, so the
// values are append only.
//
// What is deliberately NOT here: any bit layout for the stream profile. The
// profile is derived from the mesh asset's own vertex layout structure by the
// single conversion function on the C++ side, and that function is the only
// place the encoding is written down. Shader code never shifts profile bits: it
// tests the named VF_HAS_* flags the mapper emits, which is what keeps this file
// from having to be edited whenever the engine's layout encoding changes.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// VF_TYPE values
//
// The name is the payload; the numeric value is the identity that ends up in the
// variant key and in the variant name.
//
// VF_TYPE is a variant axis: it is supplied by the assembler through the define
// set of the compilation (Base included), not defaulted here. There is no
// #ifndef fallback on purpose -- a missing VF_TYPE makes every
// #if VF_TYPE == ... branch false, so the failure is a loud "VFAttributes not
// declared" instead of a silent compilation of the wrong vertex factory.
// ---------------------------------------------------------------------------

#define VF_TYPE_ENGINE_MESH					0
// 1 is retired: it used to be reserved for a separate skeletal factory, and static and
// skinned meshes are one factory now (the skin stream is an attribute of a mesh, not an
// identity of its own). No artifact was ever compiled with the value, so nothing points
// at it, and it is never handed out again either -- the numbering is append only.
#define VF_TYPE_INSTANCED_STATIC_MESH		2
#define VF_TYPE_PROCEDURAL_GRASS			3
#define VF_TYPE_FULLSCREEN					4
#define VF_TYPE_IMGUI						5
#define VF_TYPE_COUNT						6

// ---------------------------------------------------------------------------
// VF_STREAM_PROFILE
//
// The attribute presence set of a compilation, derived from the mesh asset's
// vertex layout by the C++ conversion function. It goes into the variant key,
// the variant name, and the JSON input stream conditions.
//
// The shader side must not decode it bit by bit. Use the VF_HAS_* flags below.
// ---------------------------------------------------------------------------

#ifndef VF_STREAM_PROFILE
#define VF_STREAM_PROFILE					0
#endif	//VF_STREAM_PROFILE

// ---------------------------------------------------------------------------
// VF_HAS_* presence flags
//
// Emitted by the mapper for every compilation, always as a complete set: the
// flags that are true get 1 and the others get 0.
//
// Emitting only the true ones is a trap, not a shortcut. The flags are what the
// JSON input stream conditions are evaluated against, so a flag that is simply
// missing evaluates to 0 and silently culls the matching input. The generated
// FinalInputStream then loses a member that VFAttributes still declares, the
// reflected InputLayout no longer matches the compiled input signature, and the
// pipeline state built from it will not match the shader.
//
// The defaults below are a documentation of that contract, not a safety net:
// they exist so every #if has a named source, and they are all 0.
// ---------------------------------------------------------------------------

#ifndef VF_HAS_POSITION
#define VF_HAS_POSITION						0
#endif	//VF_HAS_POSITION

#ifndef VF_HAS_NORMAL
#define VF_HAS_NORMAL						0
#endif	//VF_HAS_NORMAL

#ifndef VF_HAS_TANGENT
#define VF_HAS_TANGENT						0
#endif	//VF_HAS_TANGENT

#ifndef VF_HAS_BITANGENT
#define VF_HAS_BITANGENT					0
#endif	//VF_HAS_BITANGENT

#ifndef VF_HAS_UV0
#define VF_HAS_UV0							0
#endif	//VF_HAS_UV0

#ifndef VF_HAS_UV1
#define VF_HAS_UV1							0
#endif	//VF_HAS_UV1

#ifndef VF_HAS_COLOR
#define VF_HAS_COLOR						0
#endif	//VF_HAS_COLOR

#ifndef VF_HAS_SKIN_INDEX
#define VF_HAS_SKIN_INDEX					0
#endif	//VF_HAS_SKIN_INDEX

#ifndef VF_HAS_SKIN_WEIGHT
#define VF_HAS_SKIN_WEIGHT					0
#endif	//VF_HAS_SKIN_WEIGHT

#ifndef VF_HAS_INSTANCE_STREAM
#define VF_HAS_INSTANCE_STREAM				0
#endif	//VF_HAS_INSTANCE_STREAM

// ---------------------------------------------------------------------------
// Metadata table
//
// Consumed by the C++ requirement resolver, not by shader code. It is written
// down here because this file is the single place a new vertex factory is
// registered. Required attributes must be a subset of what the mesh declares or
// the resolver fails hard; optional attributes fall back to a constant when the
// mesh does not carry them.
//
// The families on the left are the engine's own vertex layout families, which is
// what the conversion function reads; there is no second vocabulary.
//
//   VF_TYPE                        Required                              Optional
//   -----------------------------  ------------------------------------  ----------------------------------
//   VF_TYPE_ENGINE_MESH            Position                              TexCoord, Normal, Tangent, UV1, Color,
//                                                                         SkinIndex + SkinWeight
//   VF_TYPE_INSTANCED_STATIC_MESH  Position, TexCoord, InstanceStream    Normal, Tangent, Color
//   VF_TYPE_PROCEDURAL_GRASS       Position (handle)                     -
//   VF_TYPE_FULLSCREEN             TexCoord                              -
//   VF_TYPE_IMGUI                  Position (2D), TexCoord, Color        -
//
// The first two rows are served by Components/C_VertexFactory.json, the grass row by
// the same component through its VF_TYPE branch, and the last two by components of
// their own (FullScreen/, ImGui/): a row here says which attributes a factory reads,
// not which file declares them.
//
// The engine mesh row is one row for two kinds of mesh because it is one factory: a
// skinned mesh carries the same streams as a static mesh plus a skin stream, and the
// skin stream is what selects the skinning step. SkinIndex and SkinWeight are therefore
// one optional attribute in two streams, and they are only ever present together --
// EShaderJobBuilder::ResolveVertexAxis sets both flags from the mesh's own vertex layout
// or neither.
//
// Skin is four influences wide (SHADER_VERTEX_SKIN_INFLUENCE_NUM in ShaderJobBuilder.h):
// one element of each stream is one vertex's worth of influences, and the mesh stores one
// 16 bit bone index and one float per influence. The count is not a variant axis yet, so
// the resolver refuses a mesh whose EffectBoneNum says otherwise instead of compiling a
// factory that would read the wrong bytes.
//
// Attribute 0 is typed per factory where a factory's stream is not a mesh
// position, rather than sharing one declaration: VF_TYPE_PROCEDURAL_GRASS takes a
// handle (uint, which the table allows), selected by VF_TYPE in
// Components/C_VertexFactory.json. A factory whose whole stream layout is its own
// gets no entry there at all - it gets a component of its own, which is where its
// declaration lives and where the pipeline reads the stream's slot from:
// FullScreen/C_FullScreenVertexFactory.json declares one UV at slot 0, and
// ImGui/C_ImGuiVertexFactory.json declares the interleaved ImDrawVert at slot 0.
// Neither program lists the generic component any more, and no VF_TYPE branch for
// either factory is left in it. That is the shape to copy for the next factory
// whose stream the mesh layout cannot describe. Every other attribute stays shared
// through its VF_HAS_* flag and needs no per factory declaration.
//
// VS only resources, culled automatically for the pixel stage because the
// generated pixel code never references them:
//
//   VF_TYPE_ENGINE_MESH            BoneMatrices, BoneInverseTransposeMatrices
//                                  (declared under VF_HAS_SKIN_WEIGHT, not under a
//                                  VF_TYPE condition: only a mesh that carries the
//                                  skin stream can reference them)
//   VF_TYPE_INSTANCED_STATIC_MESH  InstanceData
//   VF_TYPE_PROCEDURAL_GRASS       PackedInstanceDatas, DispatchDatas, LayerTypeDatas
//
// The three grass names are the ones the construct pass declares as its UAVs and
// the render side reads back as SRVs: the blade instance table, the per LOD
// instance bases, and the layer type table. This row used to spell the first two
// GrassInstanceData and GrassDispatchData and to leave the layer table out; the
// names a factory can index are the ones the buffers are declared with.
// ---------------------------------------------------------------------------
#endif	//VERTEX_FACTORY_TYPES_HLSL
