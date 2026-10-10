#ifndef MATERIAL_DOMAIN_TYPES_HLSL
#define MATERIAL_DOMAIN_TYPES_HLSL

// ---------------------------------------------------------------------------
// Material domain identities.
//
// A domain is a compile time identity, not a runtime value. It selects the
// shading implementation, the GBuffer encode and decode pair, the field set of
// MaterialSurface, and the required vertex attributes. Adding a domain is a
// three place change; see the design's domain chapter.
//
// These are identity constants, so they are plain defines and deliberately not
// #ifndef guarded: a JSON that tries to redefine one gets a redefinition
// diagnostic instead of silently winning. MATERIAL_DOMAIN, the value a given
// compilation is pinned to, is a different thing -- it is supplied by the
// domain component's Defines and is compared against these constants.
//
// The numeric values are written into GBufferD as the DomainId and are carried
// in the variant key, so they are append only.
// ---------------------------------------------------------------------------

#define MATERIAL_DOMAIN_UNLIT				0
#define MATERIAL_DOMAIN_DEFAULT_LIT			1
#define MATERIAL_DOMAIN_CLEAR_COAT			2
#define MATERIAL_DOMAIN_SHEEN				3
#define MATERIAL_DOMAIN_SUBSURFACE			4
#define MATERIAL_DOMAIN_COUNT				5

// ---------------------------------------------------------------------------
// Surface modes
//
// The other half of the material pipeline identity, and it lives in the same
// file because the two are read together: the domain says how a surface shades,
// the surface mode says how it is covered and therefore which pass draws it and
// what blend state it gets.
//
//   Opaque       no clipping, alpha ignored, depth write on, blending off
//   Mask         discard below the cutoff, depth write on, blending off
//   Translucent  forward pass, depth write off, alpha blending on, sorted
//
// Plain defines, not #ifndef guarded, for the same reason the domain ids are:
// these are the identity table, and a JSON that tries to redefine one should get
// a redefinition diagnostic rather than silently winning.
// ---------------------------------------------------------------------------

#define MATERIAL_SURFACE_MODE_OPAQUE		0
#define MATERIAL_SURFACE_MODE_MASK			1
#define MATERIAL_SURFACE_MODE_TRANSLUCENT	2
#define MATERIAL_SURFACE_MODE_COUNT			3

// Values 5..255 stay reserved for future domains.
//
// ---------------------------------------------------------------------------
// RESOLVED: there is no MATERIAL_DOMAIN_NONE.
//
// The lighting system has no concept of a none domain, so the domain list starts
// at Unlit and every table numbers from there. This table is the single source:
// the CPU side enum must drop its own NONE entry rather than keeping it and
// shifting this one, otherwise the two disagree by one and an Unlit pixel
// decodes as DefaultLit while the variant key the CPU builds fails to match the
// variant that was actually compiled.
//
// The places that share this numbering and therefore must stay in step:
//   the domain table, D_DefaultLit.json writing MATERIAL_DOMAIN = 1,
//   the sample variant defines, and the Encode / Decode dispatch switches.
// ---------------------------------------------------------------------------
#endif	//MATERIAL_DOMAIN_TYPES_HLSL
