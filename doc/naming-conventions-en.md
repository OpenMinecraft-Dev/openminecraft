[中文](naming-conventions.md)

# OpenMinecraft Naming Conventions

> Scope: all C++ sources under `include/`, `src/` and `tools/`, plus directory layout, external data keys and commit messages.
> This document describes the conventions **actually followed by the current codebase**, not an ideal state —
> known deviations are collected in [Section 12](#12-exceptions-and-legacy).
> When adding a file, type or macro, walk through the [checklist](#11-checklist).

## 1. Quick Reference

| Element | Convention | Example |
| --- | --- | --- |
| Module directory | all lowercase, hyphen-separated | `openminecraft-core/`, `openminecraft-shell/` |
| Header / source file | `om_<module>_<component>.hpp` / `.cpp`, all lowercase snake_case | `om_renderer_rendertarget.hpp` |
| Type (class / struct) | `OM` + PascalCase | `OMRendererRenderTarget`, `OMNbtTag` |
| Enum type | `OM` + PascalCase | `OMJsonNodeType` |
| Enumerator | PascalCase, no prefix | `Handshake`, `Object` |
| Namespace | `openminecraft::<module>[::submodule]` | `openminecraft::specs::zip` |
| Shell-layer namespace | `openminecraft::shell::<submodule>` | `openminecraft::shell::renderer` |
| Function / method | lowerCamelCase, verb first | `readVarInt`, `handlePlayLogin` |
| Member variable | lowerCamelCase, **no prefix, no suffix** | `overlay`, `shaderManager` |
| Local / parameter | lowerCamelCase, domain abbreviations allowed | `pck`, `hnd`, `idx` |
| Global variable | lowerCamelCase, `extern` in header | `blockRegistry`, `logAgent` |
| Constant | `constexpr` + lowerCamelCase | `topDepth` |
| Macro | `OM_` + UPPER_CASE | `OM_MEM_CPP`, `OM_PLATFORM_UNIX` |
| Include guard | uppercase file name + `_HPP` | `OM_UTIL_VERSION_HPP` |
| External data key | snake_case, or inherit the upstream format | `animation_speed`, `minecraft:grass_block` |
| Commit message | `[Tag] lowercase English phrase` | `[Renderer] timeline finish` |

## 2. Directories and Files

### 2.1 Layout

```
include/openminecraft/<module>/...         core public headers
include/openminecraft-shell/<submodule>/... shell public headers
src/openminecraft-core/<module>/...         core implementation
src/openminecraft/<submodule>/...           shell implementation
```

- The module name is the namespace name: `include/openminecraft/vfs/` maps to `openminecraft::vfs`.
- Platform-specific implementations live under `plat/<platform>/` and keep the same file name as the
  generic implementation; the build script picks one:
  `src/openminecraft-core/log/plat/desktop/om_log_plat.cpp` vs `plat/android/om_log_plat.cpp`.
- Build target names are hyphenated: `openminecraft-core`, `openminecraft-renderer`, `openminecraft-vm`.

### 2.2 File naming

- Always `om_<module>_<component>.hpp` / `.cpp`: all lowercase with underscores, **no hyphens, no capital letters**.
- One primary type per file; the `.hpp` and `.cpp` share the same name.
- **File names are concatenated, type names are word-separated.** This is deliberate — do not "align" them:

  | File | Type |
  | --- | --- |
  | `om_renderer_rendertarget.hpp` | `OMRendererRenderTarget` |
  | `om_renderer_layer_vk_swapchainmanager.hpp` | `OMSwapchainManager` |
  | `om_demiurge_element_textsdf.hpp` | `OMDemiurgeElementTextSdf` |

  Rationale: lowercase file names cannot express word boundaries; type names must remain readable.

- Module prefix per directory:

  | Directory | File prefix |
  | --- | --- |
  | `binary/` | `om_bin_` |
  | `io/` | `om_io_` |
  | `log/`, `mem/`, `network/`, `renderer/`, `util/`, `vfs/`, `world/`, `i18n/` | `om_<module>_` |
  | `specs/` | named after the format: `om_png`, `om_zip`, `om_nbt`, `om_jfif`, `om_zlib`, `om_classfile` |
  | `geom/` | named after the component: `om_font`, `om_fontset`, `om_svg_structure` |
  | `vm/` | named after the component: `om_elysia_*`, `om_atomic`, `om_bytecodes`, `om_hardware` |

### 2.3 Special files

- JNI binding files keep the Java class name: `java_io_FileDescriptor.cpp`, `sun_misc_Unsafe.cpp`.
  This is the **only** place where a non-snake_case file name is allowed, because symbols such as
  `Java_java_io_FileDescriptor_initIDs` must match it.
- Assembly files keep the platform convention: `unix_cpuinfo.S`.
- Implementations that are currently excluded from the build use the `.disabled` suffix
  (e.g. `om_pixeltower_tracing.cpp.disabled`); do not leave them inside a build glob.

## 3. Types

- `OM` + PascalCase: `OMRendererVk`, `OMElysiaKlass`, `OMWorldRegistry`, `OMBundle`.
- Leading acronyms are treated as **a single PascalCase word**:
  `OMUuid`, `OMNbtTag`, `OMSvg`, `OMJfifFile`, `OMZip`, `OMFsProviderTcp`.
- Established all-caps acronyms are kept: `OMVoxelAABB`, `OMElysiaJNIEnv`, `OMRendererOpenGL`.
  The same acronym must stay consistent within one concept family (renderer backends use `Vk` / `OpenGL`).
- Types that must match the standard library are exempt (`const_iterator`).
- Nested types may omit `OM` (`OMDemiurgeSize::Unit`, `OMChunk::const_iterator`) — they are already
  qualified by the enclosing class.

## 4. Enums

- Enum type: `OM` + PascalCase (`OMJsonNodeType`, `OMProtocolMinecraftState`, `OMModelCullSide`).
- Enumerator: PascalCase, **no prefix** (`Handshake`, `Login`, `Play`, `InstanceKlass`).
- Prefer `enum class`. All 55 existing enums are unscoped `enum` (no `enum class` is used yet), relying on
  the enclosing namespace to disambiguate. New enums with generic enumerator names (`Object`, `Array`,
  `Up`, `Error`) **must** use `enum class`, otherwise they will collide inside the same namespace.

## 5. Namespaces

- Core: `openminecraft::<module>[::submodule]`, mirroring the directory
  (`openminecraft::specs::zip`, `openminecraft::renderer::common::demiurge::node`).
- Shell: `openminecraft::shell::<submodule>` (directory `include/openminecraft-shell/`, hyphen in the
  directory, `::` in the namespace).
- Short helper namespaces inside a class or module are allowed, e.g. `namespace executor` inside
  `openminecraft::vm::elysia`.
- Codename namespaces are implementation details only
  (`openminecraft::mem::castorice`, `openminecraft::vm::pixeltower::v1::tracing`) and must not appear in
  public APIs.
- Do not write `using namespace` in headers; `.cpp` files may, but only after all `#include`s.

## 6. Functions and Methods

- lowerCamelCase, verb first, trailing return type:

  ```cpp
  auto readVarInt() -> uint32_t;
  void handlePlayLogin(...);
  virtual auto queryOcclusionShape(int bsid) -> OMVoxelShape = 0;
  ```

- Recommended verb prefixes:

  | Prefix | Purpose |
  | --- | --- |
  | `get` / `set` | plain accessors |
  | `is` / `has` / `can` | boolean queries (`isSolid`, `isInstance`) |
  | `read` / `write` | binary / stream I/O (`readU16`, `writeVarInt`) |
  | `send` / `handle` | protocol traffic (`sendHandshake`, `handleStatusResponse`) |
  | `query` | read-only renderer-side lookups (`queryOcclusionShape`, `queryPartShade`) |
  | `to*` | conversions (`toString`, `toVKVersion`, `toInstance`) |
  | `mark*` / `clear*` | state flags (`markDirty`) |

- When overriding an STL virtual function, keep the STL name (`underflow`, `seekoff`, `begin`, `end`).

## 7. Variables

### 7.1 Member variables

- lowerCamelCase, **no `m_` prefix and no `_` suffix**:

  ```cpp
  class OMComposeRenderer {
      OMRendererPipeline *mainPipeline, *overlayPipeline;
      OMRenderer *renderer;
  };
  ```

- When a constructor parameter shares the member's name, the initializer list is enough:

  ```cpp
  OMRendererTempTarget *overlay, *scene, *surface;
  // overlay(overlay), scene(scene), surface(surface) is valid and preferred
  ```

  If the constructor body still needs the member, rename the parameter (`src` → `source`) or write
  `this->` explicitly. Never rely on the parameter accidentally aliasing the member.

### 7.2 Locals and parameters

- lowerCamelCase; these domain abbreviations are accepted: `pck` (packet), `hnd` (handler),
  `buf` (buffer), `idx` (index), `ctx` (context).
- Abbreviations must not be ambiguous: `thr` used to mean both thread and threshold — spell the
  threshold one out as `threshold`.
- Loop counters and template parameters (`i`, `j`, `T`, `Ts`) are unrestricted.

### 7.3 Globals and constants

- Globals are lowerCamelCase, declared `extern` in a header and defined in a `.cpp`:
  `blockRegistry`, `blockStateRegistry`, `logAgent`, `fshandlers`, `threadMap`.
- Prefer `constexpr` + lowerCamelCase for constants: `constexpr float topDepth = 0.01f;`.
- All-caps is reserved for macros — do **not** define `constexpr` constants as `TICK_RATE`.

## 8. Macros

- Project-owned macros: `OM_` prefix + UPPER_CASE (SCREAMING_SNAKE):
  `OM_MEM_CPP`, `OM_OGL_DEF`, `OM_PLATFORM_UNIX`, `OM_VULKAN_DYNAMIC`, `OM_VERSION`.
- Limit macros to conditional compilation and code generation. Prefer the following instead:
  - constants → `constexpr`
  - inline functions → `inline` / `constexpr` functions
  - platform differences → build-system `add_defines`
- Always `#undef` a macro after use, especially in public headers (`OM_OGL_DEF` is the reference pattern).
- Do not rename third-party macros: `GLM_*`, `VULKAN_HPP_*`, `JNI_VERSION_*`, `SDL_*`.

## 9. Include Guards

- Rule: **file name without extension, uppercased, plus `_HPP`**.

  | File | Guard |
  | --- | --- |
  | `om_util_version.hpp` | `OM_UTIL_VERSION_HPP` |
  | `om_network_packet.hpp` | `OM_NETWORK_PACKET_HPP` |
  | `om_renderer_handler.hpp` | `OM_RENDERER_HANDLER_HPP` |

- Write the guard before pasting the body. Historically `OM_RENDERER_APP_HPP` and
  `OM_NETWORK_SOCKETSTREAM_HPP` came from copying an old file and forgetting to rename the guard.
- Single exception: `include/openminecraft/renderer/opengl/om_renderer_layer_opengl_funcs.hpp`
  is an include fragment (it is included twice in the same TU and uses
  `#ifdef OM_OGL_DEF / #else / #endif` to switch between declarations and assignments), so it **must not**
  have a guard.
- This project does not use `#pragma once`.

## 10. Strings and External Data

These are **external contracts** and are not rewritten according to C++ identifier rules:

- Resource identifiers keep the Minecraft format:
  `minecraft:grass_block[snowy=false]`, `minecraft:water[falling=true,level=8]`.
- JSON / style-sheet keys use snake_case: `animation_speed`, `opacity_hovered`
  (see `doc/demiurge/control.md`).
- Protocol fields keep the Minecraft protocol names; do not localize them.
- Log messages are short English sentences, starting lowercase, naming the module after its type:

  ```cpp
  logger.warn("unknown packet 0x{:02x} with length {} under state {}!", pck.id, pck.datalen(), state);
  ```

- Mixed Chinese/English comments are acceptable, but stay consistent within a file, and avoid typos
  (`inplementation`, `Occclusion`, and the like).

## 11. Checklist

Walk through this list when adding or modifying code:

- [ ] File name is `om_<module>_<component>.hpp/.cpp`, all lowercase
- [ ] Header guard matches the file name exactly
- [ ] Type name is `OM` + PascalCase
- [ ] Namespace is `openminecraft::...` (shell layer: `openminecraft::shell::...`)
- [ ] Functions / methods / members / locals are lowerCamelCase
- [ ] Members have no `m_` prefix and no `_` suffix
- [ ] Macros have the `OM_` prefix, are UPPER_CASE, and are `#undef`ed after use
- [ ] One compound word has exactly one casing across the repository
      (`BlockState`, not `Blockstate`; `registry`, not `registery`)
- [ ] Commit message follows `[Tag] lowercase English phrase`

## 12. Exceptions and Legacy

### 12.1 Intentional exceptions (do not "fix")

| Item | Reason |
| --- | --- |
| `OMElysium` (`include/openminecraft/vm/elysia/om_elysium.hpp`) | **Deliberately distinct**: `elysia` denotes the object model / specification layer (`OMElysiaKlass`, `OMElysiaOop`), while `elysium` denotes the VM instance itself. This is not a spelling inconsistency. |
| Concatenated file names vs word-separated type names | `rendertarget` ↔ `RenderTarget`, see 2.2 |
| `java_io_FileDescriptor.cpp` and other JNI files | Must match the Java class name, see 2.3 |
| `om_renderer_layer_opengl_funcs.hpp` without a guard | Include fragment, see Section 9 |
| Nested types omitting `OM` | `Unit`, `const_iterator` |
| Codename namespaces | `castorice`, `pixeltower` — implementation details only |
| Third-party macros | `GLM_*`, `VULKAN_HPP_*`, `JNI_VERSION_*` |

### 12.2 Pending migration (known deviations)

| Item | Current state | Target |
| --- | --- | --- |
| `AppInfo` (`include/openminecraft/renderer/om_renderer_layer.hpp`) | missing the `OM` prefix | `OMAppInfo` |
| Endian macros (`include/openminecraft/binary/om_bin_endians.hpp`) | `be16` / `be32` / `be64` / `bef` / `bed` / `readbe16` … | `OM_BE16` / `OM_READ_BE16` …, or turn them into `constexpr` functions |
| Memory macros (`om_mem_allocator.hpp` / `.cpp`) | `defmal` / `defmalr` | add the `OM_` prefix |
| VM macros (`om_elysia_method.hpp`, `om_elysia_field.hpp`, `om_elysium.hpp`, `om_elysia_interface_defs.hpp`, `om_elysia_executor_zero.hpp`) | `attr` / `attrf` / `registerNative` / `handleFetch` / `zeroCodeFetch*` / `IMPL_FUNCCALL` | add the `OM_` prefix |
| Optimization macro (`include/optimizations.hpp`) | `HOT_FUNC` | `OM_HOT_FUNC`, and move it under `openminecraft/util/` |
| Vulkan error macros | `VkErrorTranslate`, `VkErrLogAndThrow`, `errRet`, `DET` | add the `OM_` prefix |
| ANSI color constants (`om_log_ansi.hpp`) | `OMLogAnsi*` (24 of them) | `OM_LOG_ANSI_*`; longer term make them `constexpr` |
| `.cpp`-local macros (`op_*`, `xaload`, `accessRead*`, `PUSHARG*`, `STRERR`, `PTRERR`, `OFFSWITCH`, `update`, `appendArg`, `updateFrame`, `CURRENT_KLASS`, `ARGTYPE_CASE0`) | no `OM_` prefix | add the `OM_` prefix |
| Historical commit tags | `[Rednerer]`, `[Rederer & Shell]` | do not rewrite history; use `[Renderer]` / `[Shell]` going forward |

## 13. Tooling

- `.clang-format`: `BasedOnStyle: Microsoft`, `SortIncludes: false`. Formatting only — it does not enforce naming.
- `.clang-tidy`: currently enables only `modernize-*` and `cppcoreguidelines-pro-type-cstyle-cast`;
  **no naming checks** are enabled. Suggested addition (example config, not enabled — verify against your
  local clang-tidy version):

  ```yaml
  Checks: '
    -*,
    modernize-*,
    cppcoreguidelines-pro-type-cstyle-cast,
    readability-identifier-naming
  '
  CheckOptions:
    - { key: readability-identifier-naming.NamespaceCase,         value: lower_case }
    - { key: readability-identifier-naming.ClassCase,             value: CamelCase }
    - { key: readability-identifier-naming.ClassPrefix,           value: 'OM' }
    - { key: readability-identifier-naming.StructCase,            value: CamelCase }
    - { key: readability-identifier-naming.EnumCase,              value: CamelCase }
    - { key: readability-identifier-naming.EnumConstantCase,      value: CamelCase }
    - { key: readability-identifier-naming.FunctionCase,          value: camelBack }
    - { key: readability-identifier-naming.MemberCase,            value: camelBack }
    - { key: readability-identifier-naming.VariableCase,          value: camelBack }
    - { key: readability-identifier-naming.GlobalVariableCase,    value: camelBack }
    - { key: readability-identifier-naming.ConstantCase,          value: camelBack }
    - { key: readability-identifier-naming.MacroDefinitionCase,   value: UPPER_CASE }
    - { key: readability-identifier-naming.MacroDefinitionPrefix, value: 'OM_' }
    - { key: readability-identifier-naming.IgnoredRegexp,         value: '(const_iterator|Unit|AppInfo)' }
  ```

- `misc/header_generate.py` generates JNI forwarding code — keep the generated function names aligned with
  the Java class names.
- Minimum pre-commit regression:
  `xmake build -y` plus `xmake run openminecraft network localhost 25565` (expected to exit on `unknown packet`).

## 14. Change Log

| Date | Change |
| --- | --- |
| 2026-10-17 | First cleanup pass: file renames (`registery` → `registry`, `debugrendrer` → `om_debug_renderer`, …), `om_` prefixes and the `openminecraft::shell` namespace for the shell layer, spelling fixes, `Blockstate` → `BlockState`, include guards aligned with file names, `OM` prefixes on types, trailing underscores removed from members |
