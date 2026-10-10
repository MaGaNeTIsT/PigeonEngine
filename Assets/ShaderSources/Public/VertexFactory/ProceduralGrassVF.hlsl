#ifndef PROCEDURAL_GRASS_VF_HLSL
#define PROCEDURAL_GRASS_VF_HLSL

// ---------------------------------------------------------------------------
// Procedural grass vertex factory (VF_TYPE_PROCEDURAL_GRASS).
//
// A blade is not a mesh. Its input stream is a single 32 bit handle that says
// which point of which side of the blade this vertex is, and everything else --
// where the blade stands, how tall it is, how the wind has bent it this frame --
// comes from the packed instance buffer the construct pass wrote. Every position
// this factory produces is therefore already in world space, and the draw has no
// object transform to apply: an instance of the grass is a place in the world,
// not a mesh somewhere in an object's space.
//
// Required attributes : Position (a handle, not a position)
// Optional attributes : none
// VS only resources   : PackedInstanceDatas, DispatchDatas and LayerTypeDatas,
//                       plus the per pass constant buffer BezierGrassRenderParams.
//                       All four are declared by Components/C_VertexFactory.json,
//                       which is the component of this factory. The dispatch
//                       buffer is read, not merely declared: it carries the
//                       per LOD instance base that turns the draw's own instance
//                       number into an index into the packed instance buffer,
//                       see VFUnpackVertex.
//
// ---------------------------------------------------------------------------
// How this factory relates to the three function contract
//
// It is expressed through the contract rather than around it, and the reason is
// the middle function:
//
//   VFUnpackVertex      turns the handle, the instance entry and the layer type
//                       row into one blade: the curve point, the normal and
//                       tangent the blade faces with, and the blade coordinates.
//                       What it writes to the local members are the blade's own
//                       values, and for this factory those values are world space
//                       values, because a blade has no object and no instance
//                       space to be local to.
//
//   VFTransformToWorld  composes local -> instance -> world, and for this factory
//                       that composition is the identity: there is no object
//                       matrix and no instance transform between the blade and
//                       the world. So it copies the local values into the world
//                       members, which is the same thing FullScreenVF.hlsl does
//                       for the opposite reason -- a factory with no world space
//                       fills the world members with its local values, and a
//                       factory with no local space fills the local members with
//                       its world values. Neither factory has to leave the
//                       contract to say so, which is why the escape gate is not
//                       needed here (see the registry note in
//                       Components/C_VertexFactory.hlsl).
//
//   VFTransformVertex   takes the world position the factory and the material
//                       agreed on, applies the material's world offset and
//                       projects, then writes the system interpolants.
//
// The material takes part in the middle as it does for every other factory: the
// vertex stage of the grass material is the shared neutral vertex material
// (VertexOps is false), which returns a zero offset, and a material that later
// wants to displace a blade returns an offset from the same hook.
//
// ---------------------------------------------------------------------------
// What the port changed
//
//   old the three register bound buffers of M_BezierGrass/BezierGrassRenderCommon.hlsl
//       (StructuredBuffer<uint> PackedInstanceDatas : register(t0), DispatchDatas
//       : register(t1), LayerTypeDatas : register(t2), spelled out in the file)
//       new the same three names, declared once in Components/C_VertexFactory.json
//       with the same slots. Nothing in this file declares a binding. The names
//       are the ones the shared library already reads, which is the point: the
//       construct pass writes PackedInstanceDatas and this factory reads the same
//       buffer under the same name, so "the same buffer, read only" is visible in
//       the source rather than hidden behind an alias.
//   old the #define ladder that pointed the library at a binding
//       (BezierGrassLayerTypeBinding and its two siblings)
//       new nothing. The library already reads the declared names directly, and a
//       resource has one name now.
//   old the render parameter bundle was never filled: the legacy render material
//       was a stub whose vertex factory returned zero for every member, so there
//       is no legacy InitBezierGrassRenderParameter to port. It is written here,
//       next to its only caller, the way the construct pass writes
//       InitBezierGrassConstructParameter next to the compute entry point.
//   old the draw's instance number used directly as the index into the packed
//       instance buffer, which is what the legacy proxy's draw implies and what
//       the construct pass's own write index contradicts
//       new the dispatch buffer's per LOD base is added to it. The construct pass
//       writes blade n of LOD l at l * XNumInstances * YNumInstances + n and
//       writes the same product into the dispatch row of l, while the indirect
//       args it fills leave the start instance location at 0 and the legacy draw
//       call passes 0 there too. So the number D3D reports counts from the start
//       of one LOD's block, and the global index the buffer is written with is
//       the one the dispatch row names plus that number. This is the reason
//       CurrentLODIndex and the dispatch buffer are both declared for this
//       factory; if the renderer is ever changed to pass the base as the start
//       instance location, GetBezierGrassBladeInstance is the one place to change.
//   old BezierGrassBladeLocalPosition took the view vector as an
//       FDFVector3 PreViewTranslation and the legacy render path fed it an
//       uninitialised Matrix
//       new the same function, handed GetCameraWorldPosition(ViewPosition). The
//       new library had already replaced the transform plus translation with the
//       plain subtraction the live revision used, so what it needs is the camera
//       position, and ViewParams.ViewPosition is the engine's own spelling of it.
//       This is also where the legacy render path's one mul(vector, matrix), the
//       TransformLocalToTranslatedWorld of BezierGrassRenderCommon.hlsl, went: it
//       is not ported at all rather than flipped, because the matrix it was given
//       was never filled in and the revision the library was migrated from had
//       already dropped the call.
//   old TRANSFORM of every blade vertex through the per draw world matrix
//       new none. The world matrix of the draw is not read here, because the
//       instance data is already world space; the clip space product below is the
//       only matrix product in the file and it is mul(M, v).
//   old the whole vertex factory was a stub (M_BezierGrass/VertexFactory.hlsl
//       returned a zeroed varying and VFHelpers.hlsl was empty)
//       new this file. The blade maths itself is not reimplemented: every step
//       below is a call into Public/BezierGrass/BezierGrassCommon.hlsl.
//
// One thing the render side picks and the legacy cannot confirm: the shared
// BezierGrassBladeLocalPosition takes a single time relevance bundle, and the
// constructor packs two (the current frame's wind and the previous frame's).
// This port passes the current one. Blending the two over the frame fraction is
// what carrying both is for, and it would take a second evaluation of the blade
// function squeezed into one bundle, so it is left as it was found and recorded
// as the one behaviour the render half could not be recovered from.
// ---------------------------------------------------------------------------

// The render side of the shared library: this factory binds the instance buffer
// as a read only SRV, so the writer half is compiled out and the readers, which
// the same buffer serves through a StructuredBuffer, stay. The library documents
// the same define under this same name.
#define BEZIER_GRASS_RENDER_ONLY	1

#include "Public/VertexFactory/VertexFactoryTypes.hlsl"
#include "Public/VertexFactory/VertexFactoryCommon.hlsl"
#include "Public/Core/ShaderSpaceTransform.hlsl"
#include "Public/BezierGrass/BezierGrassCommon.hlsl"

// Mirrors the conditional Inputs declaration of Components/C_VertexFactory.json
// verbatim. The semantic name, index and component count have to match the
// generated FinalInputStream and the reflection InputLayout, or the input layout
// the renderer builds will not match the compiled input signature. The handle is
// a uint, which is what the design's input stream table allows for this slot.
struct VFAttributes
{
	uint	Position		: POSITION0;
};

// Copies the per pass constant buffer into the render side parameter bundle,
// which is the shape the shared library works with. This is the only place in
// the factory that touches the loose constant buffer globals, and it is the same
// split the construct pass uses: the buffer holds the values grouped by the
// vector they arrived in, the bundle holds them by meaning.
//
// Only the members the render half reads are filled. The wind model, the random
// seeds, the density and the tile arithmetic are the construct pass's business;
// the wind has already been folded into the packed instance data by the time a
// blade is drawn, so what is left for the render side is where the blade stands
// and how tall it is.
BezierGrassParameter InitBezierGrassRenderParameter()
{
	BezierGrassParameter Result = (BezierGrassParameter)0;

	// y and z of the group are the LOD range, x and w belong to the construct pass.
	Result.LODMinMax = (float2)uint2(NumAllocatedLODMaxNumInstances.y, NumAllocatedLODMaxNumInstances.z);
	Result.NumTiles = TileXYNumTiles.zw;
	Result.NumSubTiles = SubTileXYNumSubTiles.zw;
	Result.TileAnchor = TileAnchorSize.xy;
	Result.TileSize = TileAnchorSize.zw;
	Result.SubTileSize = SubTileSizeBorderSize.xy;
	Result.BorderSize = SubTileSizeBorderSize.zw;

	// The row stride of the layer type table. Without it the layer type index the
	// instance carries cannot be turned into a row.
	Result.LayerTypeNumTotalElems = LayerTypeElemsBaseCustomTotalNumTypes.z;

	// Which LOD this draw is for. It is not used by the blade maths -- the packed
	// instance carries its own LOD -- but it selects the row of the dispatch
	// buffer that holds this LOD's instance base.
	Result.CurrentLODIndex = CurrentLODIndex;

	// Where along the bent blade the two halves meet.
	Result.BentBezierT = BentBezierT;

	return Result;
}

// One blade's packed instance. The draw covers one LOD and starts its instance
// count at zero, and the construct pass writes the indirect args the same way
// (the start instance location in DrawIndirectDatas is written as 0), so the
// instance id D3D reports counts from the beginning of that LOD's block rather
// than from the beginning of the buffer. The dispatch buffer holds the base of
// each LOD's block, which is what turns the reported id into the global index
// the construct pass used when it wrote the entry.
BezierGrassPackedInstanceData GetBezierGrassBladeInstance(in uint InInstanceID, in uint InCurrentLODIndex)
{
	const BezierGrassDispatchData DispatchData = GetBezierGrassDispatchData();

	return GetPackedInstanceData(DispatchData.InstanceOffset[InCurrentLODIndex] + InInstanceID);
}

void VFUnpackVertex(in VFAttributes InAttributes, in VFContext InContext, out VFVertex OutVertex)
{
	// Which point of the blade, and on which side of it, this vertex is.
	BezierGrassUnpackedVertexInputData BladeVertexData = UnpackedBezierGrassPosition(InAttributes.Position);

	BezierGrassParameter RenderParameter = InitBezierGrassRenderParameter();

	BezierGrassPackedInstanceData PackedBladeInstance = GetBezierGrassBladeInstance(InContext.InstanceID, RenderParameter.CurrentLODIndex);

	// The tile the blade grew in, and what the instance says about the blade
	// itself. The two unpack steps are split because the second one needs the
	// layer type row, and the layer type row is addressed by the index the first
	// one produced.
	BezierGrassTileInstanceData BladeTileData = (BezierGrassTileInstanceData)0;
	BezierGrassUnpackedInstanceData BladeInstanceData = (BezierGrassUnpackedInstanceData)0;
	UnpackBezierGrassInstanceDataIndexInfo(RenderParameter, PackedBladeInstance, BladeTileData, BladeInstanceData);

	BezierGrassLayerTypeBaseData LayerTypeBaseData = GetBezierGrassLayerTypeBaseData(BladeInstanceData.LayerTypeIndex, RenderParameter.LayerTypeNumTotalElems);

	UnpackBezierGrassInstanceDataPropertyInfo(RenderParameter, PackedBladeInstance, BladeTileData, LayerTypeBaseData, BladeInstanceData);

	// The blade itself: the curve point, the frame it faces with, and the two
	// coordinates the pixel stage will read back. The camera position is what the
	// function needs to decide which face of the blade faces the eye, and it is
	// also what keeps the two sides of a blade lit consistently as the view
	// rotates around it.
	BezierGrassVertexData BladeData = BezierGrassBladeLocalPosition(LayerTypeBaseData, RenderParameter, BladeTileData,
		BladeInstanceData, BladeInstanceData.CurrentTimeRelevanceData, BladeVertexData, GetCameraWorldPosition(ViewPosition));

	// Local space. The blade is authored in world space, so these local values are
	// the world values; VFTransformToWorld copies rather than transforms them.
	OutVertex.ObjectPosition = BladeData.Position;
	OutVertex.ObjectNormal = BladeData.Normal;
	OutVertex.ObjectTangent = float4(BladeData.GeomTangent, 1.0);

	// The blade's own texture coordinates: x is the side coordinate, y is the
	// position along the blade. The material reads y for its root to tip gradient.
	OutVertex.UV0 = float2(BladeData.CurveUV, BladeData.BezierT);
	OutVertex.UV1 = float2(0.0, 0.0);

	// The factory declares no optional attributes, so every member it does not
	// produce gets its documented constant instead of a conditional.
	OutVertex.Color = float4(1.0, 1.0, 1.0, 1.0);

	// Instance space. There is no instance stream: a blade instance is a place in
	// the world, and the value the contract documents for a factory without one is
	// the local value.
	OutVertex.InstancePosition = OutVertex.ObjectPosition;
	OutVertex.InstanceNormal = OutVertex.ObjectNormal;

	// Filled by VFTransformToWorld, and left at zero here so that a factory bug
	// that skipped the step shows as a blade at the origin rather than as a blade
	// with a plausible position.
	OutVertex.WorldPosition = float3(0.0, 0.0, 0.0);
	OutVertex.WorldNormal = float3(0.0, 0.0, 1.0);
	OutVertex.WorldTangent = float3(1.0, 0.0, 0.0);
	OutVertex.WorldBitangent = float3(0.0, 1.0, 0.0);

	// Free channels. The grass has no per instance colour and no second custom
	// value, so both keep the documented defaults.
	OutVertex.Custom[0] = float4(1.0, 1.0, 1.0, 1.0);
	OutVertex.Custom[1] = float4(0.0, 0.0, 0.0, 0.0);
}

void VFTransformToWorld(inout VFVertex InOutVertex, in VFContext InContext)
{
	// The identity composition, spelled out. A blade has no object matrix and no
	// instance transform, so local is world; the tangent frame is orthogonalised
	// on the way through, which is the one piece of work this step does for every
	// factory that has a tangent frame to hand.
	InOutVertex.WorldPosition = InOutVertex.ObjectPosition;
	InOutVertex.WorldNormal = InOutVertex.ObjectNormal;
	BuildWorldTangentFrame(InOutVertex.ObjectNormal, InOutVertex.ObjectTangent, InOutVertex.WorldTangent, InOutVertex.WorldBitangent);
}

void VFTransformVertex(in VFVertex InVertex, in VFContext InContext, in float3 InWorldPositionOffset, out VFVarying OutVarying)
{
	// The material has had its say by now: the offset it returned is a world space
	// one, and it is applied before the projection, which is what makes a blade a
	// material can displace behave like any other geometry.
	const float3 FinalWorldPosition = InVertex.WorldPosition + InWorldPositionOffset;

	// The one matrix product of this factory, matrix first, no transpose.
	OutVarying.Position = TransformWorldToClip(float4(FinalWorldPosition, 1.0), ViewProjectionMatrix);

	OutVarying.WorldPosition = FinalWorldPosition;
	OutVarying.WorldNormal = InVertex.WorldNormal;
	OutVarying.WorldTangent = InVertex.WorldTangent;
	OutVarying.WorldBitangent = InVertex.WorldBitangent;
	OutVarying.UV0 = InVertex.UV0;
	OutVarying.UV1 = InVertex.UV1;
	OutVarying.Color = InVertex.Color;
}
#endif	//PROCEDURAL_GRASS_VF_HLSL
