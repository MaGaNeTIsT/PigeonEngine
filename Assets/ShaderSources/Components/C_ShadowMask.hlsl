#ifndef C_SHADOW_MASK_HLSL
#define C_SHADOW_MASK_HLSL

// ---------------------------------------------------------------------------
// Body of the directional shadow mask component.
//
// Declared by the pass that resolves the cascade shadows into a screen space target, and by
// nothing else. The bindings come from the JSON; this file is the place the injection point
// expands and the place the contract is written down.
//
//   ShadowMaskParams
//     InvMaskSize   the reciprocal of the mask's pixel size. The mask covers the view, and its
//                   resolution is a scale of the view's, so the pass converts its own pixel
//                   coordinate with this value rather than with the view's InvScreenSize: when
//                   the scale is ever something other than one, only this value changes.
//
// Everything else the pass needs is per shadow slot and arrives in the slot table
// (Components/C_Shadows.json): the projection, the tile rectangle inside the atlas, the
// cascade's far split, and the bias. A second copy here would be a second source of truth for
// a value the slot already carries per cascade, which is exactly what the per slot layout
// exists to avoid.
//
// The filter radius is a compile time define and not a value in the block. It lives on
// Components/C_Shadows.json, the component every shadow sampling program declares, because the
// tap count shapes the loop ShaderShadows.hlsl compiles and both the mask pass and the lighting
// pass sample through it. Changing the preset therefore means editing that Defines block and
// recompiling every program that includes the shadow component; the runtime quality setting is
// not read at assembly time.
// ---------------------------------------------------------------------------
#endif	//C_SHADOW_MASK_HLSL
