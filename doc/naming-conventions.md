[English](naming-conventions-en.md)

# OpenMinecraft 命名规范

> 适用范围：`include/`、`src/`、`tools/` 下的全部 C++ 源码，以及目录结构、外部数据键和提交信息。
> 本规范描述的是**当前代码库实际遵守的约定**，而非理想状态：已知偏差统一登记在[第 12 节](#12-例外与遗留)。
> 新增文件、类型或宏时，请对照[第 11 节](#11-检查清单)逐条确认。

## 1. 速查表

| 元素 | 约定 | 示例 |
| --- | --- | --- |
| 模块目录 | 全小写，连字符分隔 | `openminecraft-core/`、`openminecraft-shell/` |
| 头文件 / 源文件 | `om_<模块>_<组件>.hpp` / `.cpp`，全小写 snake_case | `om_renderer_rendertarget.hpp` |
| 类型（class / struct） | `OM` + PascalCase | `OMRendererRenderTarget`、`OMNbtTag` |
| 枚举类型 | `OM` + PascalCase | `OMJsonNodeType` |
| 枚举值 | PascalCase，不带前缀 | `Handshake`、`Object` |
| 命名空间 | `openminecraft::<模块>[::子模块]` | `openminecraft::specs::zip` |
| shell 层命名空间 | `openminecraft::shell::<子模块>` | `openminecraft::shell::renderer` |
| 函数 / 方法 | lowerCamelCase，动词开头 | `readVarInt`、`handlePlayLogin` |
| 成员变量 | lowerCamelCase，**无前缀无后缀** | `overlay`、`shaderManager` |
| 局部变量 / 参数 | lowerCamelCase，允许领域缩写 | `pck`、`hnd`、`idx` |
| 全局变量 | lowerCamelCase，头文件 `extern` | `blockRegistry`、`logAgent` |
| 常量 | `constexpr` + lowerCamelCase | `topDepth` |
| 宏 | `OM_` + 全大写 | `OM_MEM_CPP`、`OM_PLATFORM_UNIX` |
| include guard | `文件名大写_HPP` | `OM_UTIL_VERSION_HPP` |
| 外部数据键 | snake_case 或沿用上游格式 | `animation_speed`、`minecraft:grass_block` |
| 提交信息 | `[Tag] 小写英文短语` | `[Renderer] timeline finish` |

## 2. 目录与文件

### 2.1 目录分层

```
include/openminecraft/<模块>/...          核心层公开头文件
include/openminecraft-shell/<子模块>/...   shell 层公开头文件
src/openminecraft-core/<模块>/...          核心层实现
src/openminecraft/<子模块>/...             shell 层实现
```

- 模块名即命名空间名：`include/openminecraft/vfs/` 对应 `openminecraft::vfs`。
- 平台相关实现放进 `plat/<平台>/`，文件名与通用实现一致，由构建脚本二选一：
  `src/openminecraft-core/log/plat/desktop/om_log_plat.cpp`、`plat/android/om_log_plat.cpp`。
- 构建目标（target）名用连字符：`openminecraft-core`、`openminecraft-renderer`、`openminecraft-vm`。

### 2.2 文件命名

- 一律 `om_<模块>_<组件>.hpp` / `.cpp`，全小写 + 下划线，**不用连字符、不用大写字母**。
- 一个文件一个主要类型；`.hpp` 与 `.cpp` 同名。
- **文件名连写，类名分词**——这是明确约定，不要"对齐"成一样：

  | 文件 | 类 |
  | --- | --- |
  | `om_renderer_rendertarget.hpp` | `OMRendererRenderTarget` |
  | `om_renderer_layer_vk_swapchainmanager.hpp` | `OMSwapchainManager` |
  | `om_demiurge_element_textsdf.hpp` | `OMDemiurgeElementTextSdf` |

  理由：文件名只能小写，无法表达词边界；类名必须分词才可读。

- 模块前缀与目录的对应：

  | 目录 | 文件前缀 |
  | --- | --- |
  | `binary/` | `om_bin_` |
  | `io/` | `om_io_` |
  | `log/`、`mem/`、`network/`、`renderer/`、`util/`、`vfs/`、`world/`、`i18n/` | `om_<模块>_` |
  | `specs/` | 按格式命名：`om_png`、`om_zip`、`om_nbt`、`om_jfif`、`om_zlib`、`om_classfile` |
  | `geom/` | 按组件命名：`om_font`、`om_fontset`、`om_svg_structure` |
  | `vm/` | 按组件命名：`om_elysia_*`、`om_atomic`、`om_bytecodes`、`om_hardware` |

### 2.3 特殊文件

- JNI 绑定文件沿用 Java 类名：`java_io_FileDescriptor.cpp`、`sun_misc_Unsafe.cpp`。
  这是**唯一**允许非 snake_case 文件名的地方，因为 `Java_java_io_FileDescriptor_initIDs` 这类符号必须与之对应。
- 汇编文件保持平台惯例名称：`unix_cpuinfo.S`。
- 暂不编译的实现用 `.disabled` 后缀（如 `om_pixeltower_tracing.cpp.disabled`），不要留在构建 glob 里。

## 3. 类型

- 统一 `OM` + PascalCase：`OMRendererVk`、`OMElysiaKlass`、`OMWorldRegistry`、`OMBundle`。
- 首字母缩写按**一个 PascalCase 单词**处理：`OMUuid`、`OMNbtTag`、`OMSvg`、`OMJfifFile`、`OMZip`、`OMFsProviderTcp`。
- 已成习惯的全大写缩写保留：`OMVoxelAABB`、`OMElysiaJNIEnv`、`OMRendererOpenGL`。
  同一缩写在同一个概念族内必须一致（渲染后端统一用 `Vk` / `OpenGL`）。
- 必须与标准库同名的类型不受限制（`const_iterator`）。
- 类内嵌套类型可以省略 `OM`（`OMDemiurgeSize::Unit`、`OMChunk::const_iterator`），因为已被外层类限定。

## 4. 枚举

- 枚举类型：`OM` + PascalCase（`OMJsonNodeType`、`OMProtocolMinecraftState`、`OMModelCullSide`）。
- 枚举值：PascalCase，**不加前缀**（`Handshake`、`Login`、`Play`、`InstanceKlass`）。
- 优先使用 `enum class`。现有 55 个枚举**全部**是未分作用域 `enum`（尚未使用 `enum class`），值名靠外层命名空间区分；
  新增枚举若值名过于通用（`Object`、`Array`、`Up`、`Error`），必须改用 `enum class`，否则会与同命名空间的其它枚举冲突。

## 5. 命名空间

- 核心层：`openminecraft::<模块>[::子模块]`，与目录一一对应（`openminecraft::specs::zip`、`openminecraft::renderer::common::demiurge::node`）。
- shell 层：`openminecraft::shell::<子模块>`（目录 `include/openminecraft-shell/`，目录用连字符、命名空间用 `::`）。
- 类内工具命名空间允许短名，如 `openminecraft::vm::elysia` 内的 `namespace executor`。
- 代号命名空间只用于内部实现（`openminecraft::mem::castorice`、`openminecraft::vm::pixeltower::v1::tracing`），不得出现在对外 API 中。
- 头文件里不要写 `using namespace`；`.cpp` 中可以用，但必须出现在所有 `#include` 之后。

## 6. 函数与方法

- lowerCamelCase，动词开头，返回类型后置：

  ```cpp
  auto readVarInt() -> uint32_t;
  void handlePlayLogin(...);
  virtual auto queryOcclusionShape(int bsid) -> OMVoxelShape = 0;
  ```

- 推荐动词前缀：

  | 前缀 | 用途 |
  | --- | --- |
  | `get` / `set` | 普通读写 |
  | `is` / `has` / `can` | 布尔查询（`isSolid`、`isInstance`） |
  | `read` / `write` | 二进制、流式读写（`readU16`、`writeVarInt`） |
  | `send` / `handle` | 协议收发（`sendHandshake`、`handleStatusResponse`） |
  | `query` | 渲染侧只读查询（`queryOcclusionShape`、`queryPartShade`） |
  | `to*` | 转换（`toString`、`toVKVersion`、`toInstance`） |
  | `mark*` / `clear*` | 状态位（`markDirty`） |

- 覆盖 STL 虚函数时沿用 STL 名字（`underflow`、`seekoff`、`begin`、`end`）。

## 7. 变量

### 7.1 成员变量

- lowerCamelCase，**不加 `m_` 前缀，不加 `_` 后缀**：

  ```cpp
  class OMComposeRenderer {
      OMRendererPipeline *mainPipeline, *overlayPipeline;
      OMRenderer *renderer;
  };
  ```

- 构造函数参数与成员同名时用初始化列表即可：

  ```cpp
  OMRendererTempTarget *overlay, *scene, *surface;
  // overlay(overlay), scene(scene), surface(surface) 合法且推荐
  ```

  若构造体内还要使用成员，必须给参数改名（`src` → `source`）或显式写 `this->`。

### 7.2 局部变量与参数

- lowerCamelCase；允许领域内公认的缩写：`pck`（packet）、`hnd`（handler）、`buf`（buffer）、`idx`（index）、`ctx`（context）。
- 缩写不得二义：`thr` 曾同时表示 thread 与 threshold，阈值请写全 `threshold`。
- **禁止双字母占位名**：`pp`、`ll`、`ff`、`kk`、`mm`、`vv`、`tt`、`nn`、`ss`、`cc`、`ii`、`dd`、`aa`、`bb`。
  这类名字不携带语义，而且同一个名字在不同文件里含义完全不同——`pp` 既曾是 Demiurge 的"布局边界矩形"，
  也曾是渲染器的"位置"。按实际含义命名：`layoutBound`、`position`、`file`、`klass`、`method`、`field`、
  `insets`、`texture`、`document`、`constant`。
- **禁止编号变量组**：`pp2`…`pp8`、`h1`…`h4`、`l1`…`l4`、`k2`。成组对象用容器或结构体表达，不要靠数字后缀区分：

  ```cpp
  // 不要：pp、pp2、…、pp8
  const std::array<Position, 8> corners = { ... };
  corners[0] - cameraPos, corners[1] - cameraPos, ...

  // 不要：h1、h2、h3、h4
  const std::array<float, 4> cornerHeights = { ... };
  ```

  有序的单个量也不加数字：`l1..l4` 应写成 `byte0..byte3`，或直接用位运算。
  位打包与 UV/AO 的领域惯用名（`l1..l4`、`ao1..ao4`、`u0/v0`）例外，见 §12.1。
- 单字母名只用于极短作用域内的循环与数学量（`i`/`j`/`k`、坐标 `x`/`y`/`z`、差值 `dx`/`dy`/`dz`）；
  `id`、`it`、`uv`、`fd`、`ok` 视为完整单词。若一个单字母名在函数里活过十几行，就该改成全称。
- 模板参数（`T`、`Ts`）不受限制。

### 7.3 全局变量与常量

- 全局变量 lowerCamelCase，头文件 `extern` 声明、`.cpp` 定义：
  `blockRegistry`、`blockStateRegistry`、`logAgent`、`fshandlers`、`threadMap`。
- 常量优先 `constexpr` + lowerCamelCase：`constexpr float topDepth = 0.01f;`。
- 全大写只用于宏，**不要**用 `TICK_RATE` 这类写法定义 `constexpr` 常量。

## 8. 宏

- 项目自有宏：`OM_` 前缀 + 全大写（SCREAMING_SNAKE）：`OM_MEM_CPP`、`OM_OGL_DEF`、`OM_PLATFORM_UNIX`、`OM_VULKAN_DYNAMIC`、`OM_VERSION`。
- 用途限定为条件编译开关与代码生成；能用以下方式替代的不要用宏：
  - 常量 → `constexpr`
  - 内联函数 → `inline` / `constexpr` 函数
  - 平台差异 → 构建系统 `add_defines`
- 宏必须在用完后 `#undef`，尤其在公共头里（`OM_OGL_DEF` 的用法是范本）。
- 第三方宏不改名：`GLM_*`、`VULKAN_HPP_*`、`JNI_VERSION_*`、`SDL_*`。

## 9. include guard

- 规则：**文件名去扩展名后整体大写 + `_HPP`**。

  | 文件 | guard |
  | --- | --- |
  | `om_util_version.hpp` | `OM_UTIL_VERSION_HPP` |
  | `om_network_packet.hpp` | `OM_NETWORK_PACKET_HPP` |
  | `om_renderer_handler.hpp` | `OM_RENDERER_HANDLER_HPP` |

- 新增头文件时先写 guard 再粘内容——历史上 `OM_RENDERER_APP_HPP`、`OM_NETWORK_SOCKETSTREAM_HPP` 都是复制旧文件后忘改名字造成的。
- 唯一例外：`include/openminecraft/renderer/opengl/om_renderer_layer_opengl_funcs.hpp`
  是 include fragment（同一 TU 内被包含两次，用 `#ifdef OM_OGL_DEF / #else / #endif` 区分声明与赋值），**不能**加 guard。
- 本项目不使用 `#pragma once`。

## 10. 字符串与外部数据

这些是**对外契约**，不按 C++ 标识符规则改：

- 资源标识符沿用 Minecraft 原格式：`minecraft:grass_block[snowy=false]`、`minecraft:water[falling=true,level=8]`。
- JSON / 样式表键用 snake_case：`animation_speed`、`opacity_hovered`（见 `doc/demiurge/control.md`）。
- 协议字段按 Minecraft 协议原名，不做本地化改写。
- 日志文本用英文短句、小写开头，模块名用类型名：

  ```cpp
  logger.warn("unknown packet 0x{:02x} with length {} under state {}!", pck.id, pck.datalen(), state);
  ```

- 注释中英混排可以接受，但同一文件内保持一致；避免拼写错误（`inplementation`、`Occclusion` 之类）。

## 11. 检查清单

新增或修改代码时逐条确认：

- [ ] 文件名是 `om_<模块>_<组件>.hpp/.cpp`，全小写
- [ ] 头文件 guard 与文件名严格对应
- [ ] 类型名是 `OM` + PascalCase
- [ ] 命名空间是 `openminecraft::...`（shell 层为 `openminecraft::shell::...`）
- [ ] 函数 / 方法 / 成员 / 局部变量都是 lowerCamelCase
- [ ] 成员没有 `m_` 前缀、`_` 后缀
- [ ] 宏有 `OM_` 前缀且全大写，用后 `#undef`
- [ ] 局部变量没有双字母占位名（`pp`、`ll`、`ff`…），也没有 `pp2`、`h1` 这类数字后缀
- [ ] 同一复合词在全仓库只有一种大小写（`BlockState` 不是 `Blockstate`，`registry` 不是 `registery`）
- [ ] 提交信息符合 `[Tag] 小写英文短语`

## 12. 例外与遗留

### 12.1 有意例外（不要"修正"）

| 项 | 说明 |
| --- | --- |
| `OMElysium`（`include/openminecraft/vm/elysia/om_elysium.hpp`） | **刻意区分**：`elysia` 指对象模型 / 规范层（`OMElysiaKlass`、`OMElysiaOop`），`elysium` 指 VM 实例本身，二者不是拼写不一致。 |
| 文件名连写、类名分词 | `rendertarget` 对应 `RenderTarget`，见 2.2 |
| `java_io_FileDescriptor.cpp` 等 JNI 文件 | 必须与 Java 类名一致，见 2.3 |
| `om_renderer_layer_opengl_funcs.hpp` 无 guard | include fragment，见第 9 节 |
| 嵌套类型省略 `OM` | `Unit`、`const_iterator` |
| 代号命名空间 | `castorice`、`pixeltower`，仅内部实现 |
| 第三方宏 | `GLM_*`、`VULKAN_HPP_*`、`JNI_VERSION_*` |
| 位打包与图形惯用名（`om_renderer_voxel_compiler.cpp` 中的 `l1..l4`、`bl1..bl4`、`h1..h4`、`ao1..ao4`、`u0/v0/u1/v1`） | 这些名字与函数上方的位布局注释（`llll`、`hhhh` 等）逐位对应，改成容器反而看不出位域关系，属于打包函数的领域写法 |

### 12.2 待迁移（已知偏差）

| 项 | 现状 | 目标 |
| --- | --- | --- |
| `AppInfo`（`include/openminecraft/renderer/om_renderer_layer.hpp`） | 缺 `OM` 前缀 | `OMAppInfo` |
| 字节序宏（`include/openminecraft/binary/om_bin_endians.hpp`） | `be16` / `be32` / `be64` / `bef` / `bed` / `readbe16` 等 | `OM_BE16` / `OM_READ_BE16` 等，或改为 `constexpr` 函数 |
| 内存宏（`om_mem_allocator.hpp` / `.cpp`） | `defmal` / `defmalr` | `OM_` 前缀 |
| 虚拟机宏（`om_elysia_method.hpp`、`om_elysia_field.hpp`、`om_elysium.hpp`、`om_elysia_interface_defs.hpp`、`om_elysia_executor_zero.hpp`） | `attr` / `attrf` / `registerNative` / `handleFetch` / `zeroCodeFetch*` / `IMPL_FUNCCALL` | `OM_` 前缀 |
| 优化宏（`include/optimizations.hpp`） | `HOT_FUNC` | `OM_HOT_FUNC`，并移入 `openminecraft/util/` |
| Vulkan 错误宏 | `VkErrorTranslate`、`VkErrLogAndThrow`、`errRet`、`DET` | `OM_` 前缀 |
| 日志颜色常量（`om_log_ansi.hpp`） | `OMLogAnsi*`（24 个） | `OM_LOG_ANSI_*`，长期可改 `constexpr` |
| `.cpp` 内局部宏（`op_*`、`xaload`、`accessRead*`、`PUSHARG*`、`STRERR`、`PTRERR`、`OFFSWITCH`、`update`、`appendArg`、`updateFrame`、`CURRENT_KLASS`、`ARGTYPE_CASE0`） | 无 `OM_` 前缀 | `OM_` 前缀 |
| 历史提交 tag | `[Rednerer]`、`[Rederer & Shell]` | 不改写历史，后续统一用 `[Renderer]`、`[Shell]` |
| 单字母局部变量与参数 | 公共头文件中仍有约 180 个非坐标单字母参数（如 `setTextColor(glm::vec4 d)`、`samples(uint64_t n)`） | 新代码禁止；存量按文件逐步改为全称（`color`、`count`），坐标 `x`/`y` 与循环 `i`/`j` 保留 |

## 13. 工具

- `.clang-format`：`BasedOnStyle: Microsoft`、`SortIncludes: false`。只负责排版，不负责命名。
- `.clang-tidy`：目前只开 `modernize-*` 与 `cppcoreguidelines-pro-type-cstyle-cast`，**没有**开启命名检查。
  建议追加（示例配置，未启用，需按本机 clang-tidy 版本验证）：

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

- `misc/header_generate.py` 生成 JNI 转发代码时，注意生成函数名与 Java 类名保持一致。
- 提交前最小回归：`xmake build -y` + `xmake run openminecraft network localhost 25565`（预期以 `unknown packet` 退出）。

## 14. 变更记录

| 日期 | 内容 |
| --- | --- |
| 2026-10-17 | 局部变量清理：清除全部双字母占位名（`pp`/`ll`/`ff`/`kk`/`mm`/`vv`/`tt`/`nn`/`ss`/`cc`/`ii`/`dd`/`aa`/`bb`，115 处）与编号变量组（`pp2..pp8`、`h1..h4`、`l1..l4`、`k2`），改用语义名或 `std::array` |
| 2026-10-17 | 首批统一：文件重命名（`registery` → `registry`、`debugrendrer` → `om_debug_renderer` 等）、shell 层补 `om_` 前缀并改用 `openminecraft::shell` 命名空间、拼写修正、`Blockstate` → `BlockState`、include guard 对齐文件名、类型补 `OM` 前缀、去掉成员尾下划线 |
