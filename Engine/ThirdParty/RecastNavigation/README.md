# Recast Navigation (vendored)

Source copy of [recastnavigation](https://github.com/recastnavigation/recastnavigation),
upstream commit `9f4ce64458dfae86e1239c525ddc219c4e9e06f1` (v1.6.0 tag lineage), zlib
licensed. See `License.txt`.

## What is copied

| Directory | Contents | Files |
|---|---|---|
| `Recast/` | Navmesh generation from triangle geometry | 11 `.cpp`, 3 headers |
| `Detour/` | Navmesh storage, tile management, A* queries | 7 `.cpp`, 9 headers |
| `DetourCrowd/` | Local steering and agent avoidance | 6 `.cpp`, 6 headers |
| `DetourTileCache/` | Dynamically rebuilt navmesh tiles | 2 `.cpp`, 2 headers |

Only `Include/` and `Source/` are copied. `RecastDemo` (SDL2 + OpenGL), `Tests` (Catch2),
`DebugUtils` and the upstream per-module `CMakeLists.txt` files are not vendored: the
engine draws navigation debug geometry through `RDebugWireframePrimitiveManager`, and
the build is described by the local `CMakeLists.txt` instead.

Upstream sources are unmodified. Do not patch them here — fix or wrap in
`Engine/EngineThirdParty/RecastNavigation/` instead, so a future upstream refresh stays a
plain copy.

## Build

`CMakeLists.txt` creates four static libraries named `Recast`, `Detour`, `DetourCrowd`
and `DetourTileCache`, built as C++98 with the engine's five configurations and the same
static runtime (`/MTd` for Debug/DebugEditor/Develop/DevelopEditor, `/MT` for Shipping).
The engine adds this directory with `add_subdirectory` from the root `CMakeLists.txt`.

Configuration macros deliberately left undefined:

- `DT_POLYREF64` — poly and tile references stay 32 bit.
- `DT_VIRTUAL_QUERYFILTER` — the default `dtQueryFilter` is used, crowd filters included.

`RC_DISABLE_ASSERTS` is defined for Shipping only, and as a `PUBLIC` definition on
`Recast` and `Detour`, because the macro is read by `RecastAssert.h` and `DetourAssert.h`
and must be identical in the libraries and in every translation unit including them.

## Include convention

The include directory is the root of this folder, so engine code includes headers with
their module path:

```cpp
#include <Recast/Include/Recast.h>
#include <Detour/Include/DetourNavMesh.h>
```

Only `Engine/EngineThirdParty/RecastNavigation/CppFiles/*.cpp` may include these headers.
Engine headers forward declare `dtNavMesh` / `dtNavMeshQuery` / `rcContext` instead, so no
third party type reaches gameplay or editor code.
