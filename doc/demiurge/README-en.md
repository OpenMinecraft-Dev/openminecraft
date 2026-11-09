[中文](README.md)

# Demiurge UI API Reference

> Scope: the node, element and control APIs plus their style keys under `include/openminecraft/renderer/common/demiurge/` and `src/openminecraft-core/renderer/common/demiurge/`.
>
> **Documentation sync rule: whenever a public API or style key of any node, element or control is added or changed, this document and [README.md](README.md) must be updated in the same commit.** A code-only change is considered incomplete.
>
> Naming conventions: [../naming-conventions-en.md](../naming-conventions-en.md).

## 1. Overview

Demiurge is an immediate-mode UI built on Yoga layout and instanced batch rendering:

```text
OMDemiurgeNode tree
  ├─ style key/value storage (layout / rendering / control tuning)
  ├─ Yoga layout  →  layoutBound (absolute screen rect)
  ├─ submit()     →  element channels (rect / roundedrect / svg / image / sector / textsdf / shaderscene)
  └─ update()     →  advance animations and write results back to own/child styles
        ↓
OMDemiurgeRendererHandler
  ├─ per frame: layout → submit → update → channel upload
  ├─ record render task (grouped by depth and clip; dynamic scissor)
  └─ applyClip(): logical rect → target-pixel clip rect
```

Three extension layers:

| Layer | Responsibility | Main sources |
| --- | --- | --- |
| **node** | layout, events, animation, child composition, element submission | `node/`, `node/container/` |
| **element** | per-draw instance data (GPU vertex format) | `element/` |
| **control** | node + elements + animation + interaction, business-facing | `node/controls/` |

## 2. Frame Lifecycle and Coordinates

`OMDemiurgeRendererHandler` runs the following every frame:

1. `layout(uiWidth, uiHeight)`: Yoga layout; `syncLayoutAll()` feeds layout keys into Yoga, `syncBoundary()` writes the absolute rect back to `layoutBound`;
2. `submit(handler, bottomDepth)`: nodes submit their `layoutBound` and rendering keys as element instances, and call `handler->applyClip(...)` for child clips;
3. `update()`: recursively update children; controls advance animations and write styles here;
4. channel `update()` uploads instance data;
5. task recording: layers by depth, groups by identical clip rects, and calls `task->setScissor` before draws.

Coordinates and scaling:

- Layout space is `logicalExtent / uiScale` (`renderer->setUiScale()` supports fractional values such as 1.5);
- Mouse coordinates are divided by `uiScale` before entering the UI;
- Clip rects are converted centrally by the handler: `extent / (logicalExtent / uiScale)`;
- `clipNode` rects are intersected along the ancestor chain;
- Clip changes trigger a task re-record (newly mounted pane children are clipped synchronously from `submit()` in the same frame).

## 3. Common Node API

Base class: `OMDemiurgeNode` (`include/openminecraft/renderer/common/demiurge/om_demiurge_node.hpp`).

### 3.1 Tree

| API | Description |
| --- | --- |
| `mount(child)` | Mount a child, returns `shared_ptr<OMDemiurgeNode>` (`this`) |
| `mountDirect(child)` | Mount without returning a value, for constructors |
| `umount(child)` | Unmount and call the child's `remove()` |
| `store(child)` | Child stores itself into a target pointer for chained construction |
| `children` | Direct child list |

### 3.2 Styles

| API | Description |
| --- | --- |
| `style(key, value)` | Write one style, returns `this` for chaining |
| `style({{key, value}, ...})` | Batch write |
| `stylesStorage` | `OMDemiurgeStyles`: `put / get<T>(key[, default]) / has / solve / isModified` |
| `solve()` | Clear the modified flag (called after submit/update) |

`put` sets `modified = true`. Layout keys are consumed by `layout()`, rendering keys are read by the node's `submit()`.

### 3.3 Layout queries

| API | Description |
| --- | --- |
| `layout(width, height)` | Run Yoga layout with the given available size |
| `boundary()` | Layout rect relative to the parent |
| `fetchLayoutBound()` | Absolute screen rect from the last layout (`OMDemiurgeRect`) |
| `syncLayoutAll()` / `syncLayout()` | Feed styles into Yoga (internal) |
| `syncBoundary(x, y)` | Compute and write back `layoutBound` (internal) |
| `isClipBoundary()` | Whether the node manages clipping for its own subtree; panes return `true` and stop ancestor recursion |
| `fetchChildren()` | Read-only list of direct children |

### 3.4 Lifecycle and rendering

| API | Description |
| --- | --- |
| `submit(handler, depth)` | Pure virtual; submit elements and clips |
| `update()` | Virtual; recursively updates children by default |
| `remove()` | Pure virtual; remove elements from channels |
| `setVisible(bool)` / `isVisible()` | Invisible nodes skip hit-testing and event dispatch; submission is decided by the owner (TabMultiPage submits only the active page) |
| `invalidate()` | Force the node to re-run style-driven submit logic (used when elements were removed and the node becomes visible again) |

### 3.5 Events

| API | Description |
| --- | --- |
| `acceptEvent(const OMDemiurgeEvent &)` | Unified entry; mouse hit-tests layout bounds, keyboard dispatches along the focus chain |
| `processMouseDown / Move / Up / Enter / Exit / Scroll` | Mouse events returning `Handled` / `Ignored` |
| `processKeyDown / KeyUp` | Keyboard events |
| `processTextInput / TextEditing` | Text input and IME composition |
| `processFocusGained / FocusLost` | Focus transitions |

### 3.6 Focus and mouse capture

| API | Description |
| --- | --- |
| `setFocusable(bool)` | Mark focusable (clickable and part of the focus chain) |
| `requestFocus()` / `releaseFocus()` | Request / release focus |
| `hasFocus()` | Whether the node is in the focus chain |
| `captureMouse()` / `releaseMouse()` | Mouse capture; keeps receiving Move/Up while dragging |
| `containsPoint(x, y)` | Whether absolute coordinates are inside the current layout rect |

### 3.7 Clipping

| API | Description |
| --- | --- |
| `setClipNode(node)` | Set the clip source; `nullptr` disables clipping |
| `fetchClipNode()` | Get the clip node (locks the `weak_ptr`) |
| `resolveClipBound()` | Intersect all ancestor clip rects |

Clip rects are screen-space rectangles (no rounded corners, no rotation), implemented by the renderer scissor.

## 4. Style Reference

### 4.1 Value types

| Type | Construction / notes |
| --- | --- |
| `OMDemiurgeSize` | `OMDemiurgeSize::pixels(x)`, `::percent(p)`, `::fit()`, `::fill()`; literals `10_px`, `50_percent` |
| `OMDemiurgePosition` | `Relative` / `Absolute` |
| `OMDemiurgeDirection` | `Row` / `RowReverse` / `Column` / `ColumnReverse` |
| `OMDemiurgeWrap` | `Wrap` / `WrapReverse` / `NoWrap` |
| `OMDemiurgeAlign` | `Auto` / `FlexStart` / `Center` / `FlexEnd` / `Stretch` / `Baseline` / `SpaceBetween` / `SpaceAround` / `SpaceEvenly` |
| `OMDemiurgeEdgeInsets` | `{top, bottom, left, right}` (`padding` / `border`) |
| Colors | `int` packed RGBA; usually `f4ToRgba(vec4)` / `genLinear(int)` |
| `glm::vec4` | `radius` (per-corner); numeric styles |
| `glm::vec2` | `scale` (two axes), etc. |
| `std::string` | `text` / `label` / `svgPath` |
| `float` / `int` | Sizes, font height, angles, opacity, etc. |

### 4.2 Layout keys (`syncLayout` → Yoga)

| Key | Type | Notes |
| --- | --- | --- |
| `width` / `height` | `OMDemiurgeSize` | Size; `fill()` means 100% |
| `minWidth` / `minHeight` / `maxWidth` / `maxHeight` | `float` | Size constraints |
| `position` | `OMDemiurgePosition` | Relative / absolute |
| `offsetX` / `offsetY` | `OMDemiurgeSize` | Position offsets |
| `margin` | `std::array<OMDemiurgeSize, 4>` | Order: top, bottom, left, right |
| `padding` | `OMDemiurgeEdgeInsets` | Padding |
| `border` | `OMDemiurgeEdgeInsets` | Border (Yoga layout; drawing handled by the node) |
| `flexDirection` | `OMDemiurgeDirection` | Main axis |
| `flexWrap` | `OMDemiurgeWrap` | Wrapping |
| `justifyContent` | `OMDemiurgeAlign` | Main-axis alignment |
| `alignItems` / `alignSelf` / `alignContent` | `OMDemiurgeAlign` | Cross-axis alignment |
| `flexGrow` / `flexShrink` | `float` | Flex factors |
| `flexBasis` | `OMDemiurgeSize` | Base size |
| `flexGap` | `OMDemiurgeSize` | Gap (percent supported) |

### 4.3 Rendering keys

| Key | Nodes | Notes |
| --- | --- | --- |
| `color` | rect / text / svg / image / sector | Color (packed RGBA `int`) |
| `radius` | `OMDemiurgeRectNode` | Per-corner radius (`vec4`) |
| `factor` | most element nodes | Edge softness / SDF width; per-node default |
| `rotation` / `rotationPivot` | rect / image / sector / svg | Rotate around the given normal |
| `scale` | all element nodes | `glm::vec2`, around the element center, independent axes (done in shaders; no jitter) |
| `clip` / `clipRadius` | `OMDemiurgeRectNode` | Per-element pixel clip rect and corner radius |
| `text` | `OMDemiurgeTextSdfNode` / multiline | Text content |
| `textheight` | text nodes | Font height |
| `textlineheight` | multiline text | Line height |
| `label` | controls with labels | Control label (Button / CheckBox / RadioBox, etc.) |
| `svgPath` / `svgSize` | `OMDemiurgeSvgNode` | SVG path and logical size |
| `fill` | `OMDemiurgeImageNode` | `OMDemiurgeImageFillType`: Fit / Contain / Cover |
| `beginAngle` / `endAngle` | `OMDemiurgeSectorNode` | Sector start/end angle (radians) |

### 4.4 Internal and control-tuning keys

The following keys are maintained by the framework or control internals; external code should not write them:

| Key | Notes |
| --- | --- |
| `_internal.color` | Written by controls whose background color is animation-driven; `OMDemiurgeRectNode::submit` prefers it over the public `color` |
| `layoutBound` | Read-only absolute rect written back after layout (framework cache) |
| `animation_speed` | Control animation duration (seconds); most controls accept `setAnimationSpeed()` or this style |
| `hover_boost` / `press_boost` | CheckBox / RadioBox hover and press color boost |
| `press_shrink` | RadioBox dot press scale (default 0.35) |
| `box_radius` | CheckBox / RadioBox box corner radius |
| `text_offset` | Label vertical offset factor |
| `opacity_normal` / `opacity_hovered` / `opacity_clicked` | Button opacity in the three states |
| `segment_length` / `squeeze` / `min_length` / `cycle` | ProgressBar indeterminate animation parameters |

> `get<T>(key, default)` silently returns the default when the stored type does not match; there is no compile-time check. New keys must use identical write/read types.

## 5. Built-in Nodes

Paths: `node/` and `node/container/`.

| Node | Base | Constructor | Purpose / main keys |
| --- | --- | --- | --- |
| `OMDemiurgeRectNode` | `OMDemiurgeNode` | default | Rounded rect; `color/radius/factor/rotation/clip/clipRadius/scale` |
| `OMDemiurgeContainerNode` | `OMDemiurgeNode` | default | Pure container, draws nothing itself; submits children |
| `OMDemiurgePaneContainer` | `OMDemiurgeContainerNode` | default | Clips **direct children** to itself; see below |
| `OMDemiurgeTextSdfNode` | `OMDemiurgeNode` | `(OMFontSet*)` | Single-line text; `text/textheight/color/factor/scale` |
| `OMDemiurgeTextSdfMultilineNode` | `OMDemiurgeNode` | `(OMFontSet*)` | Multi-line text; plus `textlineheight` |
| `OMDemiurgeSvgNode` | `OMDemiurgeNode` | default | SDF SVG icon; `svgPath/svgSize/color/factor/rotation/scale` |
| `OMDemiurgeImageNode` | `OMDemiurgeNode` | `(std::string textureTag)` | Texture image; `fill/radius/rotation/factor/color/scale` |
| `OMDemiurgeSectorNode` | `OMDemiurgeNode` | default | Sector/ring; `beginAngle/endAngle/factor/rotation/color/scale` |
| `OMDemiurgeShaderSceneNode` | `OMDemiurgeNode` | `(std::string shader)` | Full-screen shader scene; layout and `scale` only |

### 5.1 PaneContainer

- Both `submit()` and `update()` **walk the whole subtree recursively** and reassign every descendant's `clipNode` to the pane;
- The walk stops at nodes where `isClipBoundary() == true`: a nested pane is still clipped by its parent pane, but its interior is managed recursively by the child pane itself;
- Child clip resolution intersects along the ancestor chain, so the effective clip of a nested pane is `child pane ∩ parent pane`;
- Controls that need a tighter internal clip (for example the caret/selection of TextInput / TextEdit) re-assert their own clips in their `submit()`, and the result is intersected with the pane;
- The pane draws no element; mount an `OMDemiurgeRectNode` child for a background (it is clipped to the pane too).

## 6. Element Layer (Renderer Contributors)

Elements are per-channel instance structs, not business-facing APIs. Every element carries `position = {x, y, w, h}` and `depth`, and supports `glm::vec2 scale = (1, 1)`.

| Element | Extra fields | Channel |
| --- | --- | --- |
| `OMDemiurgeElementRect` | `color`, `depth`, `scale` | `OMDemiurgeRectChannel` |
| `OMDemiurgeElementRoundedRect` | `color/radius/rotation/factor/scale/clipPosition/clipRadius` | `OMDemiurgeRoundedRectChannel` |
| `OMDemiurgeElementSvg` | `color/rotation/factor/glyphId/scale` | `OMDemiurgeSvgChannel` |
| `OMDemiurgeElementImage` | `color/radius/rotation/factor/fillMode/scale` | `OMDemiurgeImageChannel` |
| `OMDemiurgeElementSector` | `color/rotation/beginAngle/endAngle/factor/scale` | `OMDemiurgeSectorChannel` |
| `OMDemiurgeElementTextSdf` | `color/factor/glyphIndex/scale` | `OMDemiurgeTextSdfChannel` |
| `OMDemiurgeElementShaderScene` | `depth/scale` | `OMDemiurgeShaderSceneChannel` |

### 6.1 scale semantics

- `glm::vec2`, independent per axis;
- Around the element's own rect center, applied in the vertex/fragment shader;
- Layout and instance positions are untouched, so Yoga pixel snapping cannot cause jitter;
- Text nodes also scale glyph centers around the text node center, scaling the whole text block rather than shrinking each glyph in place.

### 6.2 Clipping and batches

- Nodes call `handler->applyClip(channel, index, logicalRect)` to store clips;
- Channels group by `depth` and identical clip rects before calling `task->setScissor`;
- `image` / `shaderscene` channels use a per-element pipeline and set the scissor for each draw;
- Clip changes trigger a task re-record; other frames have zero overhead.

### 6.3 Checklist for a new element

1. Add the struct under `element/` (with the `position/color/depth` convention and `scale`);
2. Append the attributes to the channel instance format (types must match the struct);
3. Handle two-axis scale in the vertex/fragment shaders (geometry or SDF sampling);
4. Read the `scale` style (and others) in the node's `submit()` and write the instance;
5. For clipping, call `handler->applyClip(...)`.

## 7. Controls

Path: `node/controls/`. Controls derive from their visual base (`OMDemiurgeRectNode` or `OMDemiurgeContainerNode`) and, except Badge / Divider, implement the common interface:

```cpp
class OMDemiurgeControl
{
  public:
    virtual ~OMDemiurgeControl() = default;
    virtual void setDisabled(bool value, bool animate = true) = 0;
};
```

This allows a uniform `std::shared_ptr<OMDemiurgeControl>` to call `setDisabled()`.

### 7.1 Common conventions

- **APIs** own runtime state and animation; **styles** own layout, static appearance and control tuning;
- Color/selection/progress setters drive `OMAnimationValue<T>` internally and write child styles in `update()`;
- `setDisabled(value, animate)`: animated transition when `animate = true`, immediate otherwise;
- Animation duration defaults to about 0.18s; override with `setAnimationSpeed(seconds)` or the `animation_speed` style (per control);
- Do not overwrite internal styles written by `update()` every frame (for example `_internal.color`);
- Do not edit internal nodes (text / SVG icon) directly; use the control APIs.

### 7.2 Button

`OMDemiurgeButton(geom::OMFontSet *fontset)`

| API | Description |
| --- | --- |
| `setBackgroundColor(glm::vec4)` | Background color (with alpha), animated |
| `setOpacity(float)` | Opacity multiplier (default 0.6; hover 0.8, press 1.0) |
| `setTextColor(glm::vec4)` | Text color |
| `setBackgroundRadius(glm::vec4)` | Background corner radius |
| `setOnClick(std::function<void()>)` | Click callback |
| `setDisabled(bool, bool animate)` | Disable |
| `isDisabled()` | Query |

Main styles: `label`, `textheight`, `radius`, `opacity_normal`, `opacity_hovered`, `opacity_clicked`, `animation_speed`.
The label is set through the `label` style (there is no `setLabel`).

### 7.3 IconButton

`OMDemiurgeIconButton(float size = 28.0f)`

| API | Description |
| --- | --- |
| `setIcon(svgPath, viewBox = 14.0f)` | SVG icon path and view box |
| `setIconColor(glm::vec4)` / `setBackgroundColor(glm::vec4)` | Icon / background colors |
| `setOnClick(...)` | Click callback |
| `setAnimationSpeed(float)` | Animation duration |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disabled state |

### 7.4 CheckBox

`OMDemiurgeCheckBox(geom::OMFontSet *fontset = nullptr)`

| API | Description |
| --- | --- |
| `setChecked(bool, bool animate = true)` / `isChecked()` | Checked state |
| `setOnChange(std::function<void(bool)>)` | Change callback |
| `setCheckColor(glm::vec4)` / `setBoxColors(unchecked, checked)` | Check and box colors |
| `setTextColor(glm::vec4)` / `setLabel(std::string)` | Label |
| `setAnimationSpeed(float)` | Animation duration |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

Main styles: `label`, `textheight`, `radius`, `svgPath` (check path), `hover_boost`, `press_boost`, `text_offset`, `animation_speed`.

### 7.5 RadioBox / RadioGroup

`OMDemiurgeRadioBox(geom::OMFontSet *fontset = nullptr)`  
`OMDemiurgeRadioGroup()`

RadioBox:

| API | Description |
| --- | --- |
| `setSelected(bool, bool animate = true)` / `isSelected()` | Selection state |
| `setGroup(group)` | Join a group (group owns single/multi-select policy) |
| `setOnChange(std::function<void(bool)>)` | Change callback |
| `setDotColor(glm::vec4)` / `setBoxColors(unselected, selected)` | Dot / box colors |
| `setTextColor` / `setLabel` | Label |
| `setAnimationSpeed(float)` | Animation duration |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

The dot scales down while pressed by `press_shrink` (default 0.35), applied in the shader.

RadioGroup:

| API | Description |
| --- | --- |
| `add(box)` | Add and mount a member |
| `select(box)` / `click(box)` | Selection / click policy (single-select or multi-select toggle) |
| `setDirection(OMDemiurgeDirection)` / `setGap(float)` | Layout |
| `setMultiSelect(bool)` / `isMultiSelect()` | Multi-select mode |

### 7.6 Switch

`OMDemiurgeSwitch()`

| API | Description |
| --- | --- |
| `setChecked(bool, bool animate = true)` / `isChecked()` | State |
| `setOnChange(std::function<void(bool)>)` | Callback |
| `setTrackColors(off, on)` / `setThumbColor(vec4)` | Colors |
| `setAnimationSpeed(float)` | Animation duration |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

### 7.7 Slider

`OMDemiurgeSlider()`

| API | Description |
| --- | --- |
| `setValue(float, bool notify = true)` / `getValue()` | Current value (0..1) |
| `setOnChange(std::function<void(float)>)` | Callback |
| `setColors(track, fill, thumb)` | Colors |
| `setAnimationSpeed(float)` | Animation duration |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

### 7.8 ProgressBar

`OMDemiurgeProgressBar()`

| API | Description |
| --- | --- |
| `setProgress(float)` | 0..1 is determinate; outside that range or NaN is indeterminate |
| `setIndeterminate(bool)` / `isIndeterminate()` | Explicitly switch/query the indeterminate state |
| `getProgress()` / `isDeterminate()` | Query |
| `setTrackColor(vec4)` / `setBarColor(vec4)` | Colors |
| `setAnimationSpeed(float)` | Animation duration |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

Indeterminate animation styles: `segment_length`, `squeeze`, `min_length`, `cycle`.

### 7.9 Spinner

`OMDemiurgeSpinner()`

| API | Description |
| --- | --- |
| `setColor(glm::vec4)` | Dot color |
| `setSpeed(float)` | Angular speed (rad/s) |
| `setDotCount(int)` | Dot count |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

### 7.10 Badge

`OMDemiurgeBadge(geom::OMFontSet *fontset = nullptr)` (derives `OMDemiurgeRectNode`; no `setDisabled`)

| API | Description |
| --- | --- |
| `setText(std::string)` | Text |
| `setBackgroundColor(vec4)` / `setTextColor(vec4)` | Colors |
| `setTextHeight(int)` | Font height |

### 7.11 Divider

`OMDemiurgeDivider(bool vertical = false)` (no `setDisabled`)

| API | Description |
| --- | --- |
| `setColor(glm::vec4)` | Line color |
| `setThickness(float)` | Thickness (writes the `width` / `height` style) |

### 7.12 Link

`OMDemiurgeLink(geom::OMFontSet *fontset = nullptr)`

| API | Description |
| --- | --- |
| `setText(std::string)` | Text |
| `setTextColor(vec4)` / `setUnderlineColor(vec4)` | Text / underline colors |
| `setOnClick(...)` | Click callback |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

### 7.13 TextInput (single-line)

`OMDemiurgeTextInput(geom::OMFontSet *fontset = nullptr)`

| API | Description |
| --- | --- |
| `setText(std::string)` / `getText()` | Text |
| `setPlaceholder(std::string)` | Placeholder |
| `setMaxLength(int)` | Maximum code points |
| `setOnChange(fn)` / `setOnSubmit(fn)` | Change / submit callbacks |
| `setTextHeight(int)` | Font height (writes a style) |
| `setColors(background, text, caret)` | Colors |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |
| `isTextInputFocused()` (static) | Whether any text control owns focus (used to suppress game keys) |

Supports mouse drag selection, Shift+arrows, Ctrl/Cmd+A, Backspace/Delete, IME composition, horizontal scrolling and animated caret movement. Selection and caret are clipped to the control itself.

### 7.14 TextEdit (multi-line)

`OMDemiurgeTextEdit(geom::OMFontSet *fontset = nullptr)` (no `setOnSubmit`)

| API | Description |
| --- | --- |
| `setText(std::string)` / `getText()` | Text (`\n` separates lines) |
| `setPlaceholder(std::string)` | Placeholder |
| `setMaxLength(int)` | Maximum code points |
| `setOnChange(fn)` | Change callback |
| `setTextHeight(int)` / `setColors(background, text, caret)` | Font height / colors |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable |

Also supports ←/→/↑/↓, Home/End, Enter for a new line, multi-line selection, vertical scrolling, per-line render virtualization, and selection/caret clipping to itself.

### 7.15 DropDownPane

`OMDemiurgeDropDownPane(geom::OMFontSet *fontset = nullptr)`

A collapsible panel: rounded-rect body, a title row with a chevron that rotates with the state, click the title row to toggle, animated height between collapsed and expanded, and content clipped by the control itself.

| API | Description |
| --- | --- |
| `setTitle(std::string)` | Title (also available as the `label` style) |
| `setContentNode(shared_ptr<OMDemiurgeNode>)` / `getContentNode()` | Set / get the content node (replaces the old content) |
| `setExpanded(bool, bool animate = true)` / `isExpanded()` | Expanded state |
| `toggle(bool animate = true)` | Toggle expanded/collapsed |
| `setOnToggle(std::function<void(bool)>)` | State change callback |
| `setHeaderHeight(float)` / `setAnimationSpeed(float)` | Header height / animation duration |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable (no click, dimmed) |

Main styles:

| Key | Description |
| --- | --- |
| `label` / `textheight` | Title text / font height |
| `color` / `radius` | Rounded-rect body background / corner radius |
| `text_color` / `arrow_color` | Title / chevron colors |
| `animation_speed` | Expand/collapse duration |
| `header_height` | Header height (also `setHeaderHeight`) |
| `header_padding` | Header padding (`OMDemiurgeEdgeInsets`) |
| `arrow_path` / `svgSize` | Chevron SVG path (downward chevron by default) |

Behavior:

- Height = `headerHeight + contentHeight * openAmount`; content is absolutely positioned below the header;
- Content is clipped to the control via `clipNode`, revealing progressively while the height animates;
- The chevron uses the SVG `rotation` style with `openAmount * π`;
- Only clicks on the header row toggle the pane; content controls keep their own interaction.

### 7.16 TabMultiPage

`OMDemiurgeTabMultiPage(geom::OMFontSet *fontset = nullptr)`

A top tab bar plus a multi-page container. Every page must be mounted through `addPage()`; only the active page is submitted, while the others stay mounted, invisible and excluded from hit-testing. Page content switching is not animated, but the tab indicator slides along the tab bar.

| API | Description |
| --- | --- |
| `addPage(title, page)` | Mount a page and return its index; the first page becomes active |
| `setActivePage(int)` / `getActivePage()` | Switch / query the active page |
| `setTabPosition(OMDemiurgeTabPosition)` / `getTabPosition()` | Tab bar position: `Top` / `Bottom` / `Left` / `Right` |
| `setTabWidth(float)` | Width of a single tab for top/bottom layouts |
| `setTabAlignment(OMDemiurgeTabAlignment)` / `getTabAlignment()` | Bar alignment: `Start` / `Center` / `End`; left/center/right for top/bottom, top/center/bottom for left/right, default `Center` |
| `setPageFill(bool)` / `isPageFill()` | Page fills the remaining space: the control needs an explicit/`height: fill`; page nodes get `height: fill` automatically |
| `setPageEasing(int, animation::OMEasingFunc)` | Easing used when that page slides in (the old page slides out with the same easing); defaults to `easeOutCubic` |
| `setPageAnimationSpeed(float)` | Page transition duration in seconds (default 0.25, 0 disables it) |
| `getPageCount()` | Page count |
| `setOnPageChange(std::function<void(int)>)` | Page change callback |
| `setDisabled(bool, bool animate)` / `isDisabled()` | Disable (tabs dimmed and not clickable) |

Main styles:

| Key | Description |
| --- | --- |
| `tab_height` / `tab_width` / `textheight` | Vertical tab row height / horizontal tab width / label font height |
| `active_text_color` / `inactive_text_color` | Active / inactive label colors |
| `indicator_color` / `indicator_height` | Underline indicator color / height |
| `bar_line_color` | Full-width divider under the tab bar |
| `animation_speed` | Indicator / disabled-state animation duration |
| `page_animation_speed` | Page transition duration in seconds |

Behavior:

- On switch the old page `remove()`s its elements and the new page's whole subtree is `invalidate()`d so elements are rebuilt;
- Inactive pages are `setVisible(false)`: they stay mounted and laid out but receive no mouse events;
- The tab bar size depends only on the tab count, not on page content:
  - Top/bottom: each tab is `tab_width` wide (default 100); bar length = `count × tab_width`, centered horizontally inside the control;
  - Left/right: each tab row is `tab_height` tall; bar length = `count × tab_height`, bar width = `max(tab_height, 64)` so vertical labels remain readable;
- For left/right the label reserves extra padding on the indicator side to avoid overlapping the indicator; control height = `max(active page height, bar height)`;
- The bar can be aligned along its own axis (`Start` / `Center` / `End`); the full-width divider follows the bar position;
- The indicator position is clamped to the actual tab range and never goes past the first/last tab (computed in the control's local space so it stays centered on the tab);
- For left/right layouts the vertical divider only covers the tab bar, not the full page height;
- On switch the old page is not removed immediately: both pages are submitted and slide along the tab bar axis, and the old page is `remove()`d when the animation ends;
  - Top/bottom layouts slide horizontally, left/right layouts slide vertically;
  - When moving to a larger tab index the new page enters from the positive direction and the old page exits to the negative direction; moving back reverses it;
  - Progress advances linearly and is then shaped by the **incoming page's** `easing` (customize with `setPageEasing`, default `easeOutCubic`);
  - Switching again mid-animation snaps the previous animation to its end before starting the new one;
- With `setPageFill(true)`, the page area is the control height minus the bar (or the full control height for left/right); page nodes switch to `height: fill` and overflow is clipped by `pageClip`;
- A single indicator slides from the old page to the new one along the bar (`indicatorPosition` animation, duration from `animation_speed`) and sits on the content-facing edge;
- Top/bottom: control height = `tabHeight + active page height`; left/right: control height = active page height, width minus the bar thickness;
- The active page is clipped to the page area via `pageClip`, so content cannot overflow into the tab bar.

## 8. Examples

### 8.1 Basic node and styles

```cpp
auto panel = std::make_shared<node::OMDemiurgeRectNode>();
panel->style({
    {"width", 260_px},
    {"height", 120_px},
    {"radius", glm::vec4(8.0f)},
    {"color", (int)f4ToRgba(glm::vec4(0.1f, 0.1f, 0.12f, 0.8f))},
});

auto label = std::make_shared<node::OMDemiurgeTextSdfNode>(fontset.get());
label->style({
    {"text", "Hello"},
    {"textheight", 16},
    {"color", (int)f4ToRgba(glm::vec4(1.0f))},
    {"scale", glm::vec2(1.2f, 0.9f)},   // two-axis scale
});

panel->mount(label);
```

### 8.2 Pane clipping

```cpp
auto pane = std::make_shared<node::container::OMDemiurgePaneContainer>();
pane->style({
    {"position", OMDemiurgePosition::Absolute},
    {"width", 200_px},
    {"height", 120_px},
});
pane->mount(longContentNode);   // overflow is clipped
```

### 8.3 Controls

```cpp
auto button = std::make_shared<node::controls::OMDemiurgeButton>(fontset.get());
button->style({{"label", "OK"}, {"width", 100_px}, {"height", 30_px}});
button->setBackgroundColor(glm::vec4(0.17f, 0.17f, 0.20f, 1.0f));
button->setOpacity(0.8f);
button->setOnClick([] { /* ... */ });

std::shared_ptr<node::controls::OMDemiurgeControl> control = button;
control->setDisabled(true);
```

## 9. Documentation Maintenance Checklist

When adding or changing a control, node or element:

1. Update the corresponding sections (API tables, style tables, element table) in both this document and [README.md](README.md);
2. Add new controls to the control list in 7.2–7.16 and document:
   - Constructor and base class;
   - Public APIs;
   - Style keys read (distinguishing public keys from tuning/internal keys);
   - Whether it implements `OMDemiurgeControl`;
3. Classify new style keys correctly in 4.2 / 4.3 / 4.4;
4. Extend the element table and vertex-format notes in section 6 for new elements;
5. Update section 2 if frame lifecycle, clipping, scaling or coordinate conversion behavior changes; update 5.1 whenever pane recursion/boundary semantics change;
6. Examples must map to real APIs (prefer names used in the current code).
