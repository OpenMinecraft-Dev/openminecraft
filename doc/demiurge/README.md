[English](README-en.md)

# Demiurge UI API 参考

> 适用范围：`include/openminecraft/renderer/common/demiurge/`、`src/openminecraft-core/renderer/common/demiurge/` 下的节点（node）、元素（element）与控件（control）API，以及它们的 style 键。
>
> **文档同步约定：新增或修改任何 node / element / control 的公开 API 或 style 键时，必须在同一个提交中同步更新本文档与 [README-en.md](README-en.md)。** 只改代码不改文档视为未完成。
>
> 命名规范见 [../naming-conventions.md](../naming-conventions.md)。

## 1. 总览

Demiurge 是一套基于 Yoga 布局、实例化批渲染的即时模式 UI：

```text
OMDemiurgeNode 树
  ├─ style 键值存储（布局 / 渲染 / 控件调参）
  ├─ Yoga 布局  →  layoutBound（绝对屏幕坐标）
  ├─ submit()   →  元素通道（rect / roundedrect / svg / image / sector / textsdf / shaderscene）
  └─ update()   →  动画值推进，把结果写回自身或子节点样式
        ↓
OMDemiurgeRendererHandler
  ├─ 每帧：layout → submit → update → 通道上传
  ├─ 录制渲染 task（按深度与裁剪分组，动态 scissor）
  └─ applyClip()：逻辑坐标 → 目标像素裁剪矩形
```

三个扩展层次：

| 层 | 职责 | 主要源码 |
| --- | --- | --- |
| **node** | 布局、事件、动画、组织子节点、向通道提交元素 | `node/`、`node/container/` |
| **element** | 单次绘制的实例数据（GPU 顶点格式） | `element/` |
| **control** | 组合节点 + 元素 + 动画 + 交互，面向业务 | `node/controls/` |

## 2. 帧生命周期与坐标

`OMDemiurgeRendererHandler` 每帧顺序：

1. `layout(uiWidth, uiHeight)`：Yoga 计算布局，`syncLayoutAll()` 把布局键写入 Yoga，`syncBoundary()` 把绝对矩形写回 `layoutBound`；
2. `submit(handler, 0)`：根节点开始提交，把自己的 `layoutBound` 与渲染键提交为元素实例，并调用 `handler->applyClip(...)` 设置子元素裁剪；子节点使用 `layer + 1`；
3. `update()`：递归更新子节点；控件在这里推进动画并写回样式；
4. 通道 `update()` 上传实例数据；
5. task 录制：每个 channel 记录当前最大层，handler 从 0 到最大层逐层提交；同一层内按相同裁剪矩形分组，绘制前调用 `task->setScissor`。

坐标与缩放：

- 布局空间大小为 `logicalExtent / uiScale`（`renderer->setUiScale()`，支持 1.5 等小数）；
- 鼠标坐标进入 UI 前会除以 `uiScale`；
- 裁剪矩形由 handler 统一换算：`extent / (logicalExtent / uiScale)`；
- `clipNode` 矩形按父链求交；
- 裁剪矩形变化会触发 task 重录（同一帧内新挂载的 pane 子节点由 `submit()` 同步裁剪）。

## 3. Node 通用 API

基类：`OMDemiurgeNode`（`include/openminecraft/renderer/common/demiurge/om_demiurge_node.hpp`）

### 3.1 树结构

| API | 说明 |
| --- | --- |
| `mount(child)` | 挂载子节点，返回 `shared_ptr<OMDemiurgeNode>`（`this`） |
| `mountDirect(child)` | 挂载但不返回值，用于构造函数内 |
| `umount(child)` | 卸载并调用子节点 `remove()`；同时释放子树内的焦点与鼠标捕获 |
| `store(child)` | 子节点把自身写入目标指针，便于链式构建 |
| `children` | 直接子节点列表 |

### 3.2 子节点 ID

每个节点为自己的**直接子节点**维护一个可选 ID 表，并同时保存两个方向：

- `节点指针 -> ID`
- `ID -> 弱引用节点指针`

| API | 说明 |
| --- | --- |
| `mount(child, id)` / `mountDirect(child, id)` / `mountFront(child, id)` | 挂载时附带 ID |
| `setChildId(child, id) -> bool` | 设置/替换 ID；重复 ID 返回 `false`；空串表示清除 |
| `clearChildId(child)` | 清除该子节点的 ID |
| `getChildId(child) -> optional<string>` | 通过子节点查 ID |
| `findChildById(id) -> shared_ptr<Node>` | 通过 ID 查子节点；目标失效时返回 `nullptr` |

规则：

- ID 只在同一个父节点的直接子节点中唯一；
- `umount(child)` 会自动删除对应标签，两个方向同时清理；
- 表内保存弱引用，子节点销毁后不会延长生命周期；
- 适合给内部组件打稳定标签（如 `closeButton`、`contentHost`），避免业务层额外维护查找表。

### 3.3 样式

| API | 说明 |
| --- | --- |
| `style(key, value)` | 写入一个 style，返回 `this` 支持链式 |
| `style({{key, value}, ...})` | 批量写入 |
| `stylesStorage` | 底层 `OMDemiurgeStyles`：`put / get<T>(key[, default]) / has / solve / isModified` |
| `solve()` | 清除 modified 标记（节点提交/更新完成后调用） |

`put` 会置 `modified = true`。布局键在 `layout()` 时被消费，渲染键在节点 `submit()` 时读取。

### 3.4 布局查询

| API | 说明 |
| --- | --- |
| `layout(width, height)` | 以给定可用尺寸执行 Yoga 布局 |
| `boundary()` | 相对父节点的布局矩形 |
| `fetchLayoutBound()` | 最近一次布局得到的绝对屏幕矩形（`OMDemiurgeRect`） |
| `syncLayoutAll()` / `syncLayout()` | 把样式写入 Yoga（内部使用） |
| `syncBoundary(x, y)` | 计算并写回 `layoutBound`（内部使用） |
| `isClipBoundary()` | 该节点是否自行管理子树裁剪；Pane 返回 `true`，祖先 pane 的递归到此停止 |
| `fetchChildren()` | 只读的直接子节点列表 |

### 3.5 生命周期与渲染

| API | 说明 |
| --- | --- |
| `submit(handler, uint32_t layer)` | 纯虚；提交元素与裁剪；根节点从 0 开始，子节点 +1 |
| `update()` | 虚函数；默认递归更新子节点 |
| `remove()` | 纯虚；从通道移除元素 |
| `setVisible(bool)` / `isVisible()` | 不可见节点跳过命中测试与事件派发；是否提交由所有者决定（如 TabPane 只提交当前页） |
| `invalidate()` | 强制节点重新执行由样式驱动的提交逻辑（元素被移除后重新显示时使用） |

### 3.6 事件

| API | 说明 |
| --- | --- |
| `acceptEvent(const OMDemiurgeEvent &)` | 统一入口；鼠标按布局边界命中，键盘沿焦点链派发 |
| `processMouseDown / Move / Up / Enter / Exit / Scroll` | 鼠标事件，返回 `Handled` / `Ignored` |
| `processKeyDown / KeyUp` | 键盘事件 |
| `processTextInput / TextEditing` | 文本输入与 IME 组合 |
| `processFocusGained / FocusLost` | 焦点变化 |

### 3.7 焦点与鼠标捕获

| API | 说明 |
| --- | --- |
| `setFocusable(bool)` | 标记可聚焦（可点击并进入焦点链） |
| `requestFocus()` / `releaseFocus()` | 请求 / 释放焦点 |
| `hasFocus()` | 当前节点是否处于焦点链 |
| `captureMouse()` / `releaseMouse()` | 鼠标捕获，拖拽期间持续接收 Move/Up |
| `containsPoint(x, y)` | 绝对坐标是否落在当前布局矩形内 |

### 3.8 裁剪

| API | 说明 |
| --- | --- |
| `setClipNode(node)` | 指定裁剪范围来源；`nullptr` 表示不裁剪 |
| `fetchClipNode()` | 取回裁剪节点（`weak_ptr` 提升） |
| `resolveClipBound()` | 沿父链求出与所有祖先裁剪的交集矩形 |

裁剪矩形是屏幕空间矩形（无圆角、非旋转），由渲染器 scissor 实现。

### 3.9 调试输出

| API | 说明 |
| --- | --- |
| `typeName()` | 运行时类名（跨平台 Boost.Core demangle，去掉命名空间），每个节点类型自动返回自己的类名 |
| `styleDump()` | 当前 styles 的字符串形式：`{key=value, ...}`，按 key 排序 |
| `dumpTree()` | 从该节点开始递归输出整棵 UI 树，内部使用 `OMLogger`（logger 名 `demiurge.ui`），每个节点一行 |

输出格式：每层缩进两个空格，行内容为 `类名 {style...}`；`OMDemiurgeSize`、rect、insets、颜色、枚举等都有可读形式。

示例：

```cpp
node->dumpTree();                 // 输出整棵界面树
someNode->dumpTree();             // 只输出某个子树
someNode->typeName();             // "OMDemiurgeLabel"
someNode->styleDump();            // "{color=-1, text="Main", textHeight=14}"
```

主界面 `Misc` 页签中的 `Debug UI Tree` 按钮会调用 `node->dumpTree()`。

## 4. Style 参考

### 4.1 值类型

| 类型 | 构造 / 说明 |
| --- | --- |
| `OMDemiurgeSize` | `OMDemiurgeSize::pixels(x)`、`::percent(p)`、`::fit()`、`::fill()`；字面量 `10_px`、`50_percent` |
| `OMDemiurgePosition` | `Relative` / `Absolute` |
| `OMDemiurgeDirection` | `Row` / `RowReverse` / `Column` / `ColumnReverse` |
| `OMDemiurgeWrap` | `Wrap` / `WrapReverse` / `NoWrap` |
| `OMDemiurgeAlign` | `Auto` / `FlexStart` / `Center` / `FlexEnd` / `Stretch` / `Baseline` / `SpaceBetween` / `SpaceAround` / `SpaceEvenly` |
| `OMDemiurgeEdgeInsets` | `{top, bottom, left, right}`（`padding` / `border`） |
| 颜色 | `int`，RGBA 打包；通常用 `f4ToRgba(vec4)` / `genLinear(int)` |
| `glm::vec4` | `radius` 为四角半径；数值样式 |
| `glm::vec2` | `scale`（两轴）等 |
| `std::string` | `text` / `label` / `svgPath` |
| `float` / `int` | 尺寸、字号、角度、透明度等 |

### 4.2 布局键（`syncLayout` → Yoga）

| 键 | 类型 | 说明 |
| --- | --- | --- |
| `width` / `height` | `OMDemiurgeSize` | 尺寸；`fill()` 为 100% |
| `minWidth` / `minHeight` / `maxWidth` / `maxHeight` | `float` | 尺寸约束 |
| `position` | `OMDemiurgePosition` | 相对 / 绝对定位 |
| `offsetX` / `offsetY` | `OMDemiurgeSize` | 位置偏移 |
| `margin` | `std::array<OMDemiurgeSize, 4>` | 顺序：top, bottom, left, right |
| `padding` | `OMDemiurgeEdgeInsets` | 内边距 |
| `border` | `OMDemiurgeEdgeInsets` | 边框（Yoga 布局用，绘制由节点处理） |
| `flexDirection` | `OMDemiurgeDirection` | 主轴方向 |
| `flexWrap` | `OMDemiurgeWrap` | 换行 |
| `justifyContent` | `OMDemiurgeAlign` | 主轴对齐 |
| `alignItems` / `alignSelf` / `alignContent` | `OMDemiurgeAlign` | 交叉轴对齐 |
| `flexGrow` / `flexShrink` | `float` | flex 伸缩 |
| `flexBasis` | `OMDemiurgeSize` | 基础尺寸 |
| `flexGap` | `OMDemiurgeSize` | 子节点间距（百分比也支持） |

### 4.3 渲染键

| 键 | 适用节点 | 说明 |
| --- | --- | --- |
| `color` | rect 系 / 文本 / svg / image / sector | 颜色（RGBA int） |
| `radius` | `OMDemiurgeRectNode` | 四角半径（vec4） |
| `factor` | 多数元素节点 | 边缘软/SDF 宽度，按节点默认值 |
| `rotation` / `rotationPivot` | rect / image / sector / svg | 绕指定法线旋转 |
| `scale` | 所有元素节点 | `glm::vec2`，绕元素中心、按两轴独立缩放（着色器内完成，不抖动） |
| `clip` / `clipRadius` | `OMDemiurgeRectNode` | 元素级像素裁剪矩形与圆角 |
| `text` | `OMDemiurgeTextSdfNode` / multiline | 文本内容 |
| `textHeight` | 文本节点 | 字号 |
| `textLineHeight` | multiline 文本 | 行高 |
| `label` | 带标签控件 | 控件标签（Button / CheckBox / RadioBox 等） |
| `svgPath` / `svgSize` | `OMDemiurgeSvgNode` | SVG 路径与逻辑尺寸 |
| `fill` | `OMDemiurgeImageNode` | `OMDemiurgeImageFillType`：Fit / Contain / Cover |
| `beginAngle` / `endAngle` | `OMDemiurgeSectorNode` | 扇形起止角（弧度） |

### 4.4 内部键与控件调参键

以下键由框架或控件内部维护，外部代码不应写入：

| 键 | 说明 |
| --- | --- |
| `internal.color` | 控件动画驱动背景色时写入；`OMDemiurgeRectNode::submit` 优先于公开 `color` 读取 |
| `layoutBound` | 布局后写回的只读绝对矩形（框架内部缓存） |
| `animationSpeed` | 控件动画时长（秒）；多数控件可通过 `setAnimationSpeed()` 或 style 设置 |
| `hoverBoost` / `pressBoost` | CheckBox / RadioBox 悬停与按下的颜色增强量 |
| `pressShrink` | RadioBox 内圆按下缩放比例（默认 0.35） |
| `boxRadius` | CheckBox / RadioBox 外框圆角 |
| `textOffset` | 标签相对控件的垂直偏移系数 |
| `opacityNormal` / `opacityHovered` / `opacityClicked` | Button 在三种状态下的透明度 |
| `segmentLength` / `squeeze` / `minLength` / `cycle` | ProgressBar 不定态动画参数 |

> `get<T>(key, default)` 在类型不匹配时会静默返回默认值，没有编译期检查；新增键必须保证写入与读取类型一致。

## 5. 内置节点

路径：`node/` 与 `node/container/`。

| 节点 | 基类 | 构造 | 作用 / 主要键 |
| --- | --- | --- | --- |
| `OMDemiurgeRectNode` | `OMDemiurgeNode` | 无参 | 圆角矩形；`color/radius/factor/rotation/clip/clipRadius/scale` |
| `OMDemiurgePane` | `OMDemiurgeNode` | 无参 | 纯容器，不绘制自身；只提交子节点 |
| `OMDemiurgeTextSdfNode` | `OMDemiurgeNode` | `(OMFontSet*)` | 单行文本；`text/textHeight/color/factor/scale` |
| `OMDemiurgeTextSdfMultilineNode` | `OMDemiurgeNode` | `(OMFontSet*)` | 多行文本；额外 `textLineHeight` |
| `OMDemiurgeSvgNode` | `OMDemiurgeNode` | 无参 | SDF SVG 图标；`svgPath/svgSize/color/factor/rotation/scale` |
| `OMDemiurgeImageNode` | `OMDemiurgeNode` | `(std::string textureTag)` | 纹理贴图；`fill/radius/rotation/factor/color/scale` |
| `OMDemiurgeSectorNode` | `OMDemiurgeNode` | 无参 | 扇形/圆环；`beginAngle/endAngle/factor/rotation/color/scale` |
| `OMDemiurgeShaderSceneNode` | `OMDemiurgeNode` | `(std::string shader)` | 全屏 shader 场景；仅布局与 `scale` |

### 5.1 OMDemiurgePane

- 纯容器：不绘制自身，只提交子节点；
- 本身不强制裁剪子节点；需要裁剪时使用 `OMDemiurgeScrollPane` 的 viewport，或显式给节点 `setClipNode(...)`；
- 旧的 `OMDemiurgePaneContainer` 已删除，避免同一套裁剪语义分散在两个容器类中；
- 控件仍可在自己的 `submit()` 中声明内部裁剪（例如 TextInput / TextEdit 的光标与选区）。

## 6. Element 层（渲染贡献者）

元素是通道内的实例结构体，不是业务 API。所有元素都带 `position = {x, y, w, h}`、`depth`（`uint32_t` 层号，根为 0、每深一层 +1），并统一支持 `glm::vec2 scale = (1, 1)`。UI 管线关闭深度测试（`Always`），层号只用于提交顺序。

层号的最高位（`0x80000000`）是组标记：置位表示 overlay 组（供对话框等使用），计算层号时会被移除。overlay 组拥有独立的最大层计数，并在所有普通层之后统一渲染。可用 `om_demiurge_layer.hpp` 中的 `layerGroupBit`、`isOverlayLayer()`、`layerIndex()`、`overlayLayer()` 处理。

| 元素 | 额外字段 | 通道 |
| --- | --- | --- |
| `OMDemiurgeElementRect` | `color`、`depth`、`scale` | `OMDemiurgeRectChannel` |
| `OMDemiurgeElementRoundedRect` | `color/radius/rotation/factor/scale/clipPosition/clipRadius` | `OMDemiurgeRoundedRectChannel` |
| `OMDemiurgeElementSvg` | `color/rotation/factor/glyphId/scale` | `OMDemiurgeSvgChannel` |
| `OMDemiurgeElementImage` | `color/radius/rotation/factor/fillMode/scale` | `OMDemiurgeImageChannel` |
| `OMDemiurgeElementSector` | `color/rotation/beginAngle/endAngle/factor/scale` | `OMDemiurgeSectorChannel` |
| `OMDemiurgeElementTextSdf` | `color/factor/glyphIndex/scale` | `OMDemiurgeTextSdfChannel` |
| `OMDemiurgeElementShaderScene` | `depth/scale` | `OMDemiurgeShaderSceneChannel` |

### 6.1 scale 语义

- `glm::vec2`，两轴独立；
- 以元素自身的矩形中心为基准，在顶点/片元着色器中完成；
- 不改变布局与实例位置，因此不会因 Yoga 像素取整而抖动；
- 文本节点会同时按文本节点中心缩放字形中心，实现整段文字缩放（而不是逐字形原地缩小）。

### 6.2 裁剪与批次

- 节点通过 `handler->applyClip(channel, index, logicalRect)` 写入裁剪；
- 通道按 `depth` 层号与相同裁剪矩形分组后调用 `task->setScissor`；
- `image` / `shaderscene` 通道每个元素独立管线，逐次设置 scissor；
- 裁剪矩形变化时 handler 触发 task 重录（其余帧零开销）。

### 6.3 新增元素检查清单

1. 在 `element/` 增加结构体（含 `position/color/depth` 约定字段与 `scale`）；
2. 在对应 channel 的实例格式中追加属性（类型必须与结构体一致）；
3. 顶点/片元着色器处理两轴 scale（几何或 SDF 采样）；
4. 节点 `submit()` 读取 `scale` 等 style 并写入实例；
5. 如需裁剪，节点调用 `handler->applyClip(...)`。

## 7. 控件（Controls）

路径：`node/controls/`；`*Pane` 类（`OMDemiurgeTabPane`、`OMDemiurgeDropDownPane`、`OMDemiurgeScrollPane`）位于 `node/container/`。所有控件默认继承其视觉基类（`OMDemiurgeRectNode` 或 `OMDemiurgePane`），并（除 Badge / Divider 外）实现公共接口：

```cpp
class OMDemiurgeControl
{
  public:
    virtual ~OMDemiurgeControl() = default;
    virtual void setDisabled(bool value, bool animate = true) = 0;
};
```

因此可以用 `std::shared_ptr<OMDemiurgeControl>` 统一调用 `setDisabled()`。

### 7.1 通用约定

- **API** 负责运行期状态与动画；**style** 负责布局、静态外观与控件调参；
- 颜色/选中/进度等 setter 内部驱动 `OMAnimationValue<T>`，在 `update()` 中写入子节点样式；
- `setDisabled(value, animate)`：`animate = true` 时用缓动动画过渡，`false` 时立即生效；
- 动画时长默认多为 0.18s，可用 `setAnimationSpeed(seconds)` 或 `animationSpeed` style 覆盖（具体见各控件）；
- 控件在 `update()` 中每帧写入的内部样式（例如 `internal.color`）不要从外部覆盖；
- 迁移到 child id 的内部节点（如 Button 的 `background` / `text`、CheckBox 的 `box` / `check` / `label`、RadioBox 的 `box` / `dot` / `label`）脱离主控件样式同步；静态样式通过 `findChildById("xxx")->style(...)` 设置。

### 7.2 Button

`OMDemiurgeButton(geom::OMFontSet *fontset)`

| API | 说明 |
| --- | --- |
| `setBackgroundColor(glm::vec4)` | 背景色（含 alpha），动画过渡 |
| `setOpacity(float)` | 透明度乘数（默认 0.6；悬停 0.8、按下 1.0） |
| `setTextColor(glm::vec4)` | 文本颜色 |
| `setBackgroundRadius(glm::vec4)` | 背景圆角 |
| `setOnClick(std::function<void()>)` | 点击回调 |
| `setDisabled(bool, bool animate)` | 禁用 |
| `isDisabled()` | 查询 |

主要 style：`label`、`opacityNormal`、`opacityHovered`、`opacityClicked`、`animationSpeed`。
文本通过 `label` style 设置（没有 `setLabel`）。

内部节点 id：

| id | 说明 | 独立 style 示例 |
| --- | --- | --- |
| `background` | 背景矩形 | `findChildById("background")->style("radius", glm::vec4(4.0f))` |
| `text` | 文本 | `findChildById("text")->style("textHeight", 11)`；边距用 `margin`（`std::array<OMDemiurgeSize,4>`，顺序 top/bottom/left/right） |

`radius`、`textHeight`、`textMargin` 不再从主控件同步到内部节点。

### 7.3 IconButton

`OMDemiurgeIconButton(float size = 28.0f)`

| API | 说明 |
| --- | --- |
| `setIcon(svgPath, viewBox = 14.0f)` | SVG 图标路径与视口 |
| `setIconColor(glm::vec4)` / `setBackgroundColor(glm::vec4)` | 图标 / 背景色 |
| `setOnClick(...)` | 点击回调 |
| `setAnimationSpeed(float)` | 动画时长 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用状态 |

### 7.4 CheckBox

`OMDemiurgeCheckBox(geom::OMFontSet *fontset = nullptr)`

| API | 说明 |
| --- | --- |
| `setChecked(bool, bool animate = true)` / `isChecked()` | 勾选状态 |
| `setOnChange(std::function<void(bool)>)` | 变化回调 |
| `setCheckColor(glm::vec4)` / `setBoxColors(unchecked, checked)` | 勾与框颜色 |
| `setTextColor(glm::vec4)` / `setLabel(std::string)` | 标签 |
| `setAnimationSpeed(float)` | 动画时长 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

主要 style：`label`、`hoverBoost`、`pressBoost`、`animationSpeed`。

内部节点 id：`box`（外框）、`check`（SVG 勾）、`label`（文本）。`radius`、`svgPath`、`textHeight`、`textOffset` 通过对应子节点设置，例如：

```cpp
checkbox->findChildById("box")->style("radius", glm::vec4(6.0f));
checkbox->findChildById("check")->style("svgPath", path);
checkbox->findChildById("label")->style("textHeight", 15);
```

### 7.5 RadioBox / RadioGroup

`OMDemiurgeRadioBox(geom::OMFontSet *fontset = nullptr)`  
`OMDemiurgeRadioGroup()`

`OMDemiurgeRadioBox` 与 `OMDemiurgeRadioGroup` 使用独立头文件与实现文件。

RadioBox：

内部节点 id：`box`（外框）、`dot`（内圆）、`label`（文本）。`boxRadius`、`textHeight`、`textOffset` 通过子节点设置；`pressShrink`、`hoverBoost` 仍为主控件参数。

| API | 说明 |
| --- | --- |
| `setSelected(bool, bool animate = true)` / `isSelected()` | 选中状态 |
| `setGroup(group)` | 加入单选组（由 group 管理互斥/多选） |
| `setOnChange(std::function<void(bool)>)` | 变化回调 |
| `setDotColor(glm::vec4)` / `setBoxColors(unselected, selected)` | 内圆 / 外框颜色 |
| `setTextColor` / `setLabel` | 标签 |
| `setAnimationSpeed(float)` | 动画时长 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

内圆按下时会按 `pressShrink`（默认 0.35）缩放，动画在着色器中完成。

RadioGroup：

| API | 说明 |
| --- | --- |
| `add(box)` | 添加成员并挂载 |
| `select(box)` / `click(box)` | 选中 / 点击策略（单选或多选切换） |
| `setDirection(OMDemiurgeDirection)` / `setGap(float)` | 排布 |
| `setMultiSelect(bool)` / `isMultiSelect()` | 多选模式 |

### 7.6 Switch

`OMDemiurgeSwitch()`

| API | 说明 |
| --- | --- |
| `setChecked(bool, bool animate = true)` / `isChecked()` | 状态 |
| `setOnChange(std::function<void(bool)>)` | 回调 |
| `setTrackColors(off, on)` / `setThumbColor(vec4)` | 颜色 |
| `setAnimationSpeed(float)` | 动画时长 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

### 7.7 Slider

`OMDemiurgeSlider()`

| API | 说明 |
| --- | --- |
| `setValue(float, bool notify = true)` / `getValue()` | 当前值（0..1） |
| `setOnChange(std::function<void(float)>)` | 回调 |
| `setColors(track, fill, thumb)` | 颜色 |
| `setAnimationSpeed(float)` | 动画时长 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

### 7.8 ProgressBar

`OMDemiurgeProgressBar()`

| API | 说明 |
| --- | --- |
| `setProgress(float)` | 0..1 为确定态；越界或 NaN 为不定态 |
| `setIndeterminate(bool)` / `isIndeterminate()` | 显式切换不定态 / 查询 |
| `setError(bool, bool animate = true)` / `isError()` | 错误状态：进度条颜色平滑过渡到 `errorColor` |
| `setErrorColor(glm::vec4)` | 自定义错误颜色，默认柔和红 `vec4(0.937, 0.376, 0.400, 1)` |
| `getProgress()` / `isDeterminate()` | 查询 |
| `setTrackColor(vec4)` / `setBarColor(vec4)` | 颜色 |
| `setAnimationSpeed(float)` | 动画时长 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

不定态动画参数（style）：`segmentLength`、`squeeze`、`minLength`、`cycle`。

### 7.9 Spinner

`OMDemiurgeSpinner()`

| API | 说明 |
| --- | --- |
| `setColor(glm::vec4)` | 点颜色 |
| `setSpeed(float)` | 角速度（rad/s） |
| `setDotCount(int)` | 点数量 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

### 7.10 Badge

`OMDemiurgeBadge(geom::OMFontSet *fontset = nullptr)`（继承 `OMDemiurgeRectNode`，无 `setDisabled`）

| API | 说明 |
| --- | --- |
| `setText(std::string)` | 文本 |
| `setBackgroundColor(vec4)` / `setTextColor(vec4)` | 颜色 |
| `setTextHeight(int)` | 字号 |

### 7.11 Divider

`OMDemiurgeDivider(bool vertical = false)`（无 `setDisabled`）

| API | 说明 |
| --- | --- |
| `setColor(glm::vec4)` | 线条颜色 |
| `setThickness(float)` | 粗细（写入 `width` / `height` style） |

### 7.12 Link

`OMDemiurgeLink(geom::OMFontSet *fontset = nullptr)`

| API | 说明 |
| --- | --- |
| `setText(std::string)` | 文本 |
| `setTextColor(vec4)` / `setUnderlineColor(vec4)` | 文本 / 下划线颜色 |
| `setOnClick(...)` | 点击回调 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

### 7.13 TextInput（单行）

`OMDemiurgeTextInput(geom::OMFontSet *fontset = nullptr)`

| API | 说明 |
| --- | --- |
| `setText(std::string)` / `getText()` | 文本 |
| `setPlaceholder(std::string)` / `getPlaceholder()` | 占位符（由独立 `placeholderNode` 渲染） |
| `setPlaceholderColor(glm::vec4)` | 占位符颜色动画 |
| `setMaxLength(int)` | 最大码点数 |
| `setOnChange(fn)` / `setOnSubmit(fn)` | 变化 / 提交回调 |
| `setTextHeight(int)` | 字号（写 style） |
| `setColors(background, text, caret)` | 颜色 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |
| `isTextInputFocused()`（static） | 是否有任意文本控件持有焦点（用于屏蔽游戏按键） |

支持：鼠标拖拽选择、Shift+方向键、Ctrl/Cmd+A、Backspace/Delete、IME 组合、水平滚动、光标滑动动画。选区与光标裁剪到控件自身。

### 7.14 TextEdit（多行）

`OMDemiurgeTextEdit(geom::OMFontSet *fontset = nullptr)`（无 `setOnSubmit`）

| API | 说明 |
| --- | --- |
| `setText(std::string)` / `getText()` | 文本（`\n` 分行） |
| `setPlaceholder(std::string)` / `getPlaceholder()` | 占位符（由独立 `placeholderNode` 渲染） |
| `setPlaceholderColor(glm::vec4)` | 占位符颜色动画 |
| `setMaxLength(int)` | 最大码点数 |
| `setOnChange(fn)` | 变化回调 |
| `setTextHeight(int)` / `setColors(background, text, caret)` | 字号 / 颜色 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

额外支持：←/→/↑/↓、Home/End、Enter 换行、跨行选择、垂直滚动、按行渲染虚拟化、选区/光标裁剪到自身。

### 7.15 DropDownPane

`OMDemiurgeDropDownPane(geom::OMFontSet *fontset = nullptr)`

可展开/收起的面板：主体为圆角矩形，标题行右侧有随状态旋转的箭头，点击标题行切换，高度在收起/展开之间做动画，内容由自身裁剪。

| API | 说明 |
| --- | --- |
| `setTitle(std::string)` | 标题（也可用 `label` style） |
| `setContentNode(shared_ptr<OMDemiurgeNode>)` / `getContentNode()` | 设置 / 获取内容节点（替换旧内容） |
| `setExpanded(bool, bool animate = true)` / `isExpanded()` | 展开状态 |
| `toggle(bool animate = true)` | 切换展开/收起 |
| `setOnToggle(std::function<void(bool)>)` | 状态变化回调 |
| `setHeaderHeight(float)` / `setAnimationSpeed(float)` | 头部高度 / 动画时长 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用（不可点击并变暗） |

主要 style：

| 键 | 说明 |
| --- | --- |
| `label` / `textHeight` | 标题文本 / 字号 |
| `color` / `radius` | 圆角矩形主体背景色 / 圆角 |
| `textColor` / `arrowColor` | 标题 / 箭头颜色 |
| `animationSpeed` | 展开/收起动画时长 |
| `headerHeight` | 标题行高度（也可用 `setHeaderHeight`） |
| `headerPadding` | 标题行内边距（`OMDemiurgeEdgeInsets`） |
| `arrowPath` / `svgSize` | 箭头 SVG 路径（默认向下 chevron） |

行为说明：

- 高度 = `headerHeight + contentHeight * openAmount`，内容以绝对定位放在标题行下方；`setContentNode` 替换/移除内容后会重新测量高度（移除时收缩到标题行，添加时扩展）；
- 内容通过 `clipNode` 裁剪到控件自身，展开过程中随高度逐渐露出；
- 箭头使用 SVG 的 `rotation` style，旋转角为 `openAmount * π`；
- 只点击标题行区域才会切换，内容区的控件交互不受影响。

### 7.16 DropDownSelector

`OMDemiurgeDropDownSelector(geom::OMFontSet *fontset = nullptr)`

下拉选择器：标题行留在正常布局中；展开后的选项列表挂到根节点的 overlay 子树，使用层的最高位单独计数，排在所有普通层之后渲染，因此不会被 ScrollPane 等祖先裁剪。

| API | 说明 |
| --- | --- |
| `setItems(const std::vector<std::string> &)` / `getItems()` | 设置 / 获取选项列表 |
| `setSelectedIndex(int, bool notify = true)` / `getSelectedIndex()` | 设置 / 获取选中下标（`-1` 表示未选中） |
| `getSelectedValue()` | 当前选中文本，未选中返回空串 |
| `setPlaceholder(std::string)` | 未选中时标题行占位文本（默认 `Select...`） |
| `setHeaderColors(normal, hovered, pressed)` | 设置标题行三层颜色 |
| `setOnSelect(std::function<void(int, const std::string &)>)` | 选中回调 |
| `open()` / `close()` / `isOpen()` | 打开 / 关闭 / 查询弹层 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

行为说明：

- 选项列表由 `OMDemiurgeScrollPane` 承载，总高度上限 240px，并按完整选项行取整，避免底部出现半截选项；超出内容通过列表滚动查看；
- 每次打开时若选中项不在视口内，列表会自动滚动使其可见（尽量居中）；已可见时保持原偏移；
- 标题行背景随 normal / hover / pressed 三态做颜色动画，默认 `(0.16,0.16,0.20)`、`(0.21,0.21,0.27)`、`(0.28,0.28,0.36)`；
- 选项在鼠标悬停时用颜色动画高亮（未选中项为半透明蓝灰，选中项提亮）；列表滚动时内容在指针下移动，会清除所有选项的 hover 状态，避免悬停色残留；
- 鼠标按下后松开，且松开位置仍在选项内时才选中并关闭弹层；拖出后松开不会选中；
- 弹层打开时鼠标事件只分发给弹层（modal child），底层控件不会抢走捕获；
- 点击弹层外部（透明 backdrop）或再次点击标题行关闭；关闭采用延迟卸载，事件批次内不会立刻重新打开；
- 弹层优先显示在标题行下方；下方空间不足且上方能容纳完整列表时翻转到上方，否则限制高度并让列表内部滚动；
- 弹层展开期间，面板区域内的滚轮事件只作用于列表 ScrollPane，不再带动外层 ScrollPane；
- 弹层根节点设置 `setLayerOverlay(true)`，子节点层号自动继承最高位；overlay 组与普通层分开计数；
- 弹层挂载/卸载通过根节点的 `deferAction()` 延迟到帧边界执行，避免事件遍历期间修改 children 导致迭代器失效。

主要 style：`width` / `height`（标题行尺寸）、`padding`。选项行高暂固定 28px。

### 7.17 TabPane

`OMDemiurgeTabPane(geom::OMFontSet *fontset = nullptr)`

顶部 Tab 栏 + 多页面容器。所有页面必须通过 `addPage()` 挂载；只有当前页会被提交，其余页面保持挂载但不可见、不参与命中测试。页面内容切换没有动画，但 Tab 指示条会沿 Tab 栏滑动。

| API | 说明 |
| --- | --- |
| `addPage(title, page)` | 挂载页面并返回索引；第一个页面自动激活 |
| `setActivePage(int)` / `getActivePage()` | 切换 / 查询当前页 |
| `setTabPosition(OMDemiurgeTabPosition)` / `getTabPosition()` | Tab 栏位置：`Top` / `Bottom` / `Left` / `Right` |
| `setTabWidth(float)` | 顶部/底部布局时单个 Tab 的宽度 |
| `setTabAlignment(OMDemiurgeTabAlignment)` / `getTabAlignment()` | Tab 栏对齐：`Start` / `Center` / `End`；顶部/底部为左/中/右，左侧/右侧为上/中/下，默认 `Center` |
| `setPageFill(bool)` / `isPageFill()` | 页面填充剩余空间：控件需要显式高度/`height: fill`，页面节点自动设为 `height: fill` |
| `setPageEasing(int, animation::OMEasingFunc)` | 设置指定页面滑入时的缓动函数（旧页面按同一缓动滑出），默认 `easeOutCubic` |
| `setPageAnimationSpeed(float)` | 页面切换动画时长（秒），默认 0.25，0 表示关闭动画 |
| `getPageCount()` | 页面数量 |
| `setOnPageChange(std::function<void(int)>)` | 页面切换回调 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用（Tab 变暗、不可点击） |

主要 style：

| 键 | 说明 |
| --- | --- |
| `tabHeight` / `tabWidth` / `textHeight` | 竖排 Tab 行高 / 横排 Tab 宽度 / 文字字号 |
| `activeTextColor` / `inactiveTextColor` | 当前页 / 非当前页文字颜色 |
| `indicatorColor` / `indicatorHeight` | 下划线指示条颜色 / 高度 |
| `barLineColor` | Tab 栏底部整条分隔线颜色 |
| `animationSpeed` | Tab 指示条/禁用状态动画时长 |
| `pageAnimationSpeed` | 页面切换动画时长（秒） |
| `pageBorder` | 页面内容的边框内缩，默认 `{2,2,2,2}`，避免裁剪到边缘 |

行为说明：

- 切换页面时旧页面 `remove()` 掉自己的元素，新页面整棵子树 `invalidate()` 以重建元素；
- 非当前页 `setVisible(false)`，因此不会响应鼠标事件（但仍保持挂载与布局）；
- Tab 栏尺寸只与 Tab 数量有关，不随页面内容变化：
  - 上/下布局：每个 Tab 宽 `tabWidth`（默认 100），Tab 栏长度 = `数量 × tabWidth`，并在控件内水平居中；
  - 左/右布局：每个 Tab 行高 `tabHeight`，Tab 栏长度 = `数量 × tabHeight`，Tab 栏宽度 = `max(tabHeight, 64)`（保证竖排文字可读）；
- 左右布局时文字会为指示条让出额外内边距，避免文字与指示条重合；控件高度取 `max(当前页高度, Tab 栏高度)`；
- Tab 栏可在其方向上对齐（`Start` / `Center` / `End`），底部整宽分隔线会跟随 Tab 栏位置；
- 指示条位置会被限制在实际 Tab 范围内，不会越过首/末 Tab（使用控件局部坐标计算，保证与 Tab 居中）；
- 左/右布局的竖向分隔线只覆盖 Tab 栏范围，不再延伸到整个页面高度；
- 切换页面时旧页面不会立即移除：新旧页面同时提交并沿 Tab 栏轴向滑动，动画结束后旧页面 `remove()`；
  - 顶部/底部布局为水平滑动，左侧/右侧布局为垂直滑动；
  - 切到索引更大的 Tab 时，新页面从正向进入、旧页面向反向滑出，切回时方向相反；
  - 动画进度线性推进，再套用**目标页面**的 `easing`（可用 `setPageEasing` 自定义，默认 `easeOutCubic`）；
  - 动画过程中再次切换会先立即结束上一次动画，再开始新的切换；
- 开启 `setPageFill(true)` 后，页面区域 = 控件高度减去 Tab 栏高度（左/右布局为整个控件高度），页面节点高度自动改为 `fill`，超出部分由 `pageClip` 裁剪；
- 指示条是单独一条，沿 Tab 栏方向从旧页滑动到新页（`indicatorPosition` 动画，时长取 `animationSpeed`），并位于靠近内容的一侧；
- 顶部/底部布局：控件高度 = `tabHeight + 当前页高度`；左侧/右侧布局：控件高度 = 当前页高度，宽度减去 Tab 栏厚度；
- 当前页元素通过 `pageClip` 裁剪到页面区域，避免内容溢出到 Tab 栏。

### 7.18 ScrollBar

`OMDemiurgeScrollBar()`

由外部提供 view / content 尺寸，自动计算 thumb 的长度与位置；外观与 Tab 指示条一致（细圆角条）。为后续 `ScrollPane` 提供滚动量回调。

| API | 说明 |
| --- | --- |
| `setMetrics(viewSize, contentSize)` | 沿滚动轴的可视区尺寸 / 内容总尺寸 |
| `setViewSize(float)` / `setContentSize(float)` | 单独设置 |
| `getViewSize()` / `getContentSize()` / `getMaxScrollOffset()` | 查询 |
| `setAllowOverscroll(bool)` / `isOverscrollAllowed()` | 是否允许越界（默认允许）；关闭后 offset 自动 clamp |
| `setVisualOverscroll(float)` | 仅用于 thumb 压缩动画的外部越界量；禁止 overscroll 时可用它保留拖拽动画 |
| `setScrollOffset(float, bool notify = false)` | 设置滚动位置；允许越界时保留为 overscroll，禁止时 clamp |
| `getOverscroll()` / `isOverscrolling()` | 当前越界量 / 是否越界 |
| `getScrollOffset()` / `getThumbLength()` | 当前 offset / 当前显示 thumb 长度（含 overscroll 压缩） |
| `setOnScroll(std::function<void(float)>)` | 拖动、点击轨道或滚轮改变 offset 时回调 |
| `setOrientation(Vertical / Horizontal)` | 滚动方向，默认 `Vertical` |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |

主要 style：

| 键 | 说明 |
| --- | --- |
| `color` / `trackColor` | thumb / 轨道颜色 |
| `radius` | 圆角 |
| `minThumb` | thumb 最小长度（默认 24） |
| `scrollStep` | 滚轮步长（默认 40） |
| `overscrollCurve` | 压缩曲线指数（默认 2.0，越大越晚压缩） |
| `overscrollRange` | 压缩参考范围，以 viewSize 的倍数计（默认 1.0） |
| `width` / `height` | `Vertical` 时 width 为条宽（默认 4）、height 为轨道长度；`Horizontal` 相反 |

计算方式：

```text
thumbLength = clamp(viewSize / contentSize * trackLength, minThumb, trackLength)
maxScroll   = max(0, contentSize - viewSize)
thumbPos    = scrollOffset / maxScroll * (trackLength - thumbLength)
```

不可滚动（`contentSize <= viewSize`）时 thumb 隐藏；轨道点击会把 thumb 中心移动到点击位置，拖动与滚轮都会通过 `setOnScroll` 通知外部。

越界（`offset < 0` 或 `offset > maxScroll`）时不会把 thumb 移出轨道，而是把它贴住对应的上/下端并**非线性压缩**：

```text
overscroll        = offset < 0 ? -offset : max(0, offset - maxScroll)
t                 = clamp(overscroll / (viewSize * overscrollRange), 0, 1)
eased             = pow(t, overscrollCurve)
visualThumbLength = max(thickness, thumbLength - (thumbLength - thickness) * eased)
```

- `thickness` 为滚动条厚度（`Vertical` 时是宽度）；
- 越界接近一个 view 高度时压缩加快，`overscroll` 达到 `viewSize * overscrollRange` 时 thumb 长度等于厚度，配合自动收敛的圆角变成一个**圆点**；
- `getThumbLength()` 返回压缩后的显示长度，`setOnScroll` 仍收到原始（可能越界的）offset。

### 7.19 ScrollPane

`OMDemiurgeScrollPane()`

可滚动视口：内容挂在 `contentHost` 上并裁剪到视口，右侧/底部叠加 ScrollBar，并与内容滚动位置双向绑定。

| API | 说明 |
| --- | --- |
| `setOrientation(Horizontal / Vertical / Both)` | 滚动方向，默认 `Vertical` |
| `setContentNode(shared_ptr)` / `getContentNode()` | 设置 / 获取内容；替换内容时尽量保留原偏移，并按新的最大滚动范围 clamp（`min(offset, max)`）；移除内容时归零 |
| `setContentStretch(bool)` / `isContentStretch()` | 单轴模式下是否把内容交叉轴拉伸到视口尺寸 |
| `setScrollOffset(x, y, bool notify = false)` / `getScrollX()` / `getScrollY()` | 原始滚动位置（允许临时越界） |
| `setOnScroll(std::function<void(float, float)>)` | 拖动 / 滚轮 / 滚动条改变时回调 |
| `setDisabled(bool, bool animate)` / `isDisabled()` | 禁用 |
| `fetchVerticalScrollBar()` / `fetchHorizontalScrollBar()` | 访问内部滚动条 |

布局规则：

- `Vertical`：内容高度自由（滚动轴），宽度在 `contentStretch=true` 时为 `fill`，否则为 `fit`；
- `Horizontal`：内容宽度自由，高度在 `contentStretch=true` 时为 `fill`，否则为 `fit`；
- `Both`：宽高都自由，`contentStretch` 不生效。

主要 style：

| 键 | 说明 |
| --- | --- |
| `wheelStep` | 滚轮步长（默认 40） |
| `dragThreshold` | 开始拖拽的位移阈值（默认 4） |
| `rubberBand` | 越界拖拽阻尼系数（默认 0.45） |
| `bounceSpeed` | 越界松手回弹时长（默认 0.3） |
| `inertiaFactor` | 松手惯性预测时长（默认 0.18s） |
| `inertiaSpeed` | 惯性减速动画时长（默认 0.55s） |
| `wheelSpeed` | 滚轮平滑时长（默认 0.18） |
| `scrollbarThickness` | 内部滚动条厚度（默认 4） |
| `scrollbarGap` | 滚动条与内容之间的间隔（默认 4） |

行为说明：

- 滚轮按 `wheelStep` 平滑滚动；到达顶部/底部后直接吸附到边缘，不会因重复滚轮事件产生微抖；拖动越界时按 `rubberBand` 阻尼显示，松手后用 `easeOutCubic` 在 `bounceSpeed` 内回弹；
- 滚动条拖动会同步内容，内容拖动/滚轮会同步滚动条；
- 在合法范围内快速拖动后松手会带惯性继续滚动，按 `inertiaFactor` 预测目标、`inertiaSpeed` 减速；越界松手则执行回弹；
- 滚动条会预留独立空间：纵向条出现时内容视口宽度减去条厚，横向条出现时高度减去条厚，内容不会与滚动条重叠（Both 模式会迭代计算两条滚动条）；
- 内容尺寸变化时滚动条自动显示/隐藏；
- 按钮、输入框、DropDown 等会捕获鼠标的控件仍可正常点击；按住并拖动超过 `dragThreshold` 后 ScrollPane 会接管捕获并开始滚动；
- 若捕获鼠标的控件当前拥有焦点（Slider、Switch、TextInput 等），ScrollPane 不会接管拖拽，控件操作优先；
- ScrollBar 的拖动不会被 ScrollPane 接管，即使鼠标移出滚动条范围也会持续跟随（全局捕获）；
- ScrollPane 内部的横向/纵向 ScrollBar 都禁用实际 overscroll（`setAllowOverscroll(false)`），但会通过 `setVisualOverscroll` 接收内容的 rubber-band 量，因此拖动过度时 thumb 仍会压缩、回弹后恢复；
- 滚轮使用鼠标在窗口中的实际位置命中 ScrollPane，即使指针位于内部控件上也能滚动。

Gallery 页签现在由 ScrollPane 承载所有控件 DropDown（纵向滚动）。

### 7.20 OMDemiurgeLabel

`OMDemiurgeLabel(geom::OMFontSet *fontset = nullptr)`

`OMDemiurgeTextSdfNode` 的公开包装，放在 `node/controls/`。所有单行文本控件/页面标题均使用它。

| API | 说明 |
| --- | --- |
| `setText(std::string)` / `getText()` | 文本内容（写入 `text` style） |
| `setColor(glm::vec4)` / `getColor()` | 文本颜色；`setColor` 做 `animationSpeed`（默认 0.18s）动画 |
| `setAnimationSpeed(float)` | 颜色动画时长 |

说明：

- `getColor()` 返回当前动画中的颜色；动画结束后返回目标颜色；
- 未调用 `setColor()` 时仍可直接写 `color` style，Label 不会覆盖；
- `textHeight`、`text`、`factor` 等原有 style 继续有效；
- 文本布局/字形缓存仍由底层 `OMDemiurgeTextSdfNode` 处理。


## 8. 示例

### 8.1 基础节点与样式

```cpp
auto panel = std::make_shared<node::OMDemiurgeRectNode>();
panel->style({
    {"width", 260_px},
    {"height", 120_px},
    {"radius", glm::vec4(8.0f)},
    {"color", (int)f4ToRgba(glm::vec4(0.1f, 0.1f, 0.12f, 0.8f))},
});

auto label = std::make_shared<node::controls::OMDemiurgeLabel>(fontset.get());
label->style({
    {"text", "Hello"},
    {"textHeight", 16},
    {"color", (int)f4ToRgba(glm::vec4(1.0f))},
    {"scale", glm::vec2(1.2f, 0.9f)},   // 两轴缩放
});

panel->mount(label);
```

### 8.2 ScrollPane 裁剪

```cpp
auto pane = std::make_shared<node::container::OMDemiurgeScrollPane>();
pane->style({
    {"position", OMDemiurgePosition::Absolute},
    {"width", 200_px},
    {"height", 120_px},
});
pane->setContentNode(longContentNode);   // 超出 viewport 的部分被裁掉并可滚动
```

### 8.3 控件

```cpp
auto button = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
button->style({{"label", "OK"}, {"width", 100_px}, {"height", 30_px}});
button->setBackgroundColor(glm::vec4(0.17f, 0.17f, 0.20f, 1.0f));
button->setOpacity(0.8f);
button->setOnClick([] { /* ... */ });

std::shared_ptr<node::controls::OMDemiurgeControl> control = button;
control->setDisabled(true);
```

## 9. 文档维护清单

新增 / 修改控件、节点或元素时：

1. 在本文档与 [README-en.md](README-en.md) 中同步更新对应章节（API 表、style 表、元素表）；
2. 新控件需要加入 7.2–7.19 的控件列表，并说明：
   - 构造函数与基类；
   - 公开 API；
   - 读取的 style 键（区分公共键与调参/内部键）；
   - 若实现 `OMDemiurgeControl`，在接口说明中注明；
3. 新 style 键要归入 4.2 / 4.3 / 4.4 的正确分类；
4. 新元素要更新第 6 章的元素表与顶点格式说明；
5. 若改变帧生命周期、裁剪、缩放或坐标换算行为，更新第 2 章；Pane / ScrollPane 的裁剪语义变化时同步更新 5.1 / 7.19；
6. 文档示例必须可以对应到真实 API（优先使用当前代码中的命名）。
