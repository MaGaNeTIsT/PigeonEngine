#pragma once

namespace PigeonEngine
{

#define PE_STATIC_CONSTEXPR				static constexpr
#define PE_CONSTEXPR_STATIC				constexpr static
#define PE_CONSTEXPR					constexpr
#define PE_CONST						const
#define PE_MUTABLE						mutable
#define PE_EXTERN						extern

#define PE_CDECL						__cdecl												/* Standard C function */
#define PE_STDCALL						__stdcall											/* Standard calling convention */
#define PE_INLINE						inline												/* Function code to be inline */
#define PE_FORCEINLINE					__forceinline										/* Force code to be inline */
#define PE_FORCENOINLINE				__declspec(noinline)								/* Force code to NOT be inline */
#define PE_NODISCARD					[[nodiscard]]										/* Function returns must be used. */
#define PE_RESTRICT						__restrict											/* No alias hint */
#define PE_VARARGS						__cdecl												/* Functions with variable arguments */

#define PE_DLLEXPORT					__declspec(dllexport)								/* DLL export definitions */
#define PE_DLLIMPORT					__declspec(dllimport)								/* DLL import definitions */

}