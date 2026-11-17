# RHI Frame / Resource Upload / Sync 迁移准则

本文记录统一 RHI 的帧、资源上传、同步和资源绑定语义。目标是让 Vulkan、OpenGL 和未来原生 Metal 后端共享同一套上层规则，而不是让上层代码感知后端队列、fence、descriptor set 或 GL command table。

## 1. 目标

统一接口需要解决四件事：

1. 一帧从哪里开始、在哪里结束；
2. CPU 数据如何安全、批量地进入 GPU；
3. 上传、渲染和 present 之间如何同步；
4. 资源在什么时候可以销毁、绑定在什么时候生效。

非目标：

- 不在第一版实现完整的 render graph；
- 不强制三个后端使用完全相同的队列数量；
- 不把 `VkSemaphore`、`MTLSharedEvent`、`GLsync` 暴露给上层；
- 不承诺“修改 resource set 后旧 task 自动看到新绑定”。

## 2. 核心对象

### 2.1 `OMRendererFrameContext`

表示一帧的生命周期。

职责：

- 当前帧索引和 frames-in-flight；
- 当前帧的 `UploadBatch`；
- 当前帧的 command buffer / encoder；
- 当前帧的 fence / semaphore / timeline 状态；
- 当前帧结束后可回收的资源列表。

上层只通过它推进帧，不直接操作后端队列。

### 2.2 `OMRendererUploadBatch`

一帧内所有资源上传的统一入口。

职责：

- 接收 buffer / texture 更新请求；
- 记录 dirty region；
- 合并连续区域；
- 分配 staging ring 空间；
- 生成 barrier 和 copy 操作；
- 提交一个或多个 upload command buffer。

### 2.3 `OMRendererSyncPoint`

后端同步原语的抽象 token。

职责：

- 表达“某个操作完成后”；
- 供 graphics 等待 upload；
- 供 present 等待 graphics；
- 供 CPU 等待 frame 完成；
- 供资源回收判断是否仍在使用。

后端映射：

| 后端 | 实现 |
|---|---|
| Vulkan | `VkSemaphore` / `VkFence` / timeline semaphore |
| OpenGL | `GLsync` + `glMemoryBarrier` |
| Metal | `MTLSharedEvent` / command buffer completion |

### 2.4 `OMRendererResourceLifetime`

资源不能在 CPU 侧直接释放后立刻销毁。

规则：

```text
retire(resource, syncPoint)
  -> resource 进入延迟销毁队列
  -> syncPoint 完成后真正销毁
```

## 3. 帧生命周期

推荐顺序：

```text
Frame N:
  1. CPU 等待 frame[N].fence
  2. acquire swapchain image / drawable
  3. begin UploadBatch
  4. 记录本帧所有 buffer / texture 更新
  5. submit upload
  6. graphics submission 等待 upload sync point
  7. record + submit render tasks
  8. present
  9. signal frame fence / timeline
 10. end Frame
```

不变量：

- CPU 只在帧边界等待上一帧，不等待单个资源上传；
- 上传必须在 graphics 使用资源之前完成；
- graphics 必须在 present 之前完成；
- 复用 staging slot 前必须确认对应 frame 已完成；
- 不允许在 graphics 提交后继续写同一份 staging 数据。

## 4. 资源上传模型

### 4.1 Staging ring

不要为每个 buffer 创建一个独立 staging buffer。使用按帧划分的 staging ring：

```text
slot 0: frame 0
slot 1: frame 1
slot 2: frame 2
```

每个 slot：

- CPU 可见并持久映射；
- 由 frame context 独占；
- 对应 frame fence 未完成前不可复用；
- 支持子分配，避免每帧创建新 buffer。

### 4.2 Dirty region

UploadBatch 内部记录：

```cpp
struct UploadRegion
{
    OMRendererBuffer *buffer;
    uint64_t offset;
    uint64_t length;
};
```

规则：

- 同一 buffer 的连续区间合并；
- 同一区域多次更新只保留最后一次；
- 小更新合并为一次 copy；
- 大更新拆分，避免单帧上传占用过多 command buffer 时间；
- 区间必须按提交顺序保序，不能因合并不改变覆盖关系。

### 4.3 上传提交

UploadBatch 统一生成 upload command：

```text
for each pending buffer:
    barrier(shader read -> transfer write)
    copy(staging -> buffer)
    barrier(transfer write -> shader read)
flush / fence if required
submit
signal upload sync point
```

### 4.4 后端映射

#### Vulkan

- 使用 transfer queue；
- 一个 upload command buffer 记录所有 copy；
- transfer submit 一次；
- signal semaphore 或 timeline value；
- graphics submission 等待该 sync point；
- 资源 barrier 按 buffer/image 区域记录；
- 不每 buffer `waitIdle`。

#### OpenGL

- 没有独立 transfer queue；
- 使用 PBO ring；
- 批量 `glBufferSubData` / `glTexSubImage`；
- 使用 `glMemoryBarrier` 保证可见性；
- staging 复用前使用 `glFenceSync` + `glClientWaitSync`；
- 不每 buffer `glFinish`。

#### Metal

- 一个 command buffer；
- 一个 blit command encoder 完成所有 copy；
- 使用 `MTLSharedEvent` 连接 blit 和 render；
- staging 使用 `MTLResourceStorageModeShared` / `Managed`；
- 资源回收等待 command buffer 完成。

## 5. 同步模型

推荐按 timeline 思维设计，即使 OpenGL 使用 fence 模拟。

每帧持有：

```cpp
struct FrameSync
{
    SyncPoint uploadComplete;
    SyncPoint graphicsComplete;
    SyncPoint presentComplete;
};
```

依赖关系：

```text
uploadComplete -> graphicsComplete -> presentComplete
```

规则：

- upload 完成后 graphics 才能使用上传资源；
- graphics 完成后 present 才能读取最终图像；
- CPU 只等待 `graphicsComplete` 或 frame fence；
- 资源销毁等待 `graphicsComplete`；
- 不在 buffer update 路径等待 GPU；
- 不使用一个 fence 等待所有历史帧，除非显式重建。

### 禁止项

- 每 buffer 一次 transfer submit；
- 每 buffer 一次 fence 等待；
- 每 buffer 一次 `waitIdle`；
- 用 `glFinish` 代替 fence；
- 在 command buffer 已提交后修改其引用的 staging 数据。

## 6. ResourceSet 语义

ResourceSet 与 Pipeline 已经完全分离：

- Pipeline 只描述 shader、pipeline layout、vertex layout；
- ResourceSet 描述具体的 buffer/texture 绑定；
- Task 通过 `.pipeline(p)->resourceSet(set)` 使用。

### 6.1 绑定是 recording-time state

语义规定：

> 修改 resource set 的 bind 后，必须重新 record task；旧 task 使用旧绑定。

OpenGL task 在记录时会把资源 ID 快照进 command list，因此天然符合该语义。

Vulkan 不能只维护一个可变的 descriptor set；如果旧 task 仍引用同一个 descriptor set，那么 `vkUpdateDescriptorSets` 可能让旧 task 看到新绑定。

因此 Vulkan 需要：

- 为 recording version 保存 descriptor set 快照；或
- 每次 record 使用新的 descriptor set；
- 旧 descriptor set 在 frame fence 完成后回收；
- 不在 in-flight descriptor set 上直接更新。

### 6.2 内容更新与绑定更新

必须区分：

```cpp
// 绑定目标改变，需要重新 record
set->bindInput(1, newTexture);

// 同一 buffer 内容改变，不需要重新 record
buffer->updateData(...);
```

ResourceSet 的版本号只应在 `bindInput` 改变绑定目标时变化，不应在 buffer 内容更新时变化。

### 6.3 生命周期

- resource set 在绑定到 task 之前必须完成 bind；
- task recording 后不应再修改同一个 set，除非下一帧重新 record；
- 旧 set 的 descriptor set / table 在对应 frame 完成后回收；
- 动态绑定优先使用 per-frame set ring，而不是原地修改正在使用的 set。

## 7. 资源状态与 Barrier

建议引入轻量资源使用声明：

```cpp
enum class ResourceUsage
{
    Vertex,
    Index,
    Uniform,
    Storage,
    TextureRead,
    TransferSrc,
    TransferDst,
    RenderTarget
};
```

UploadBatch 和 task 只声明使用方式：

```cpp
batch.require(buffer, ResourceUsage::TransferDst);
batch.require(buffer, ResourceUsage::Vertex);
```

后端负责：

- Vulkan：stage / access mask / barrier；
- OpenGL：`glMemoryBarrier`；
- Metal：hazard tracking 或显式 barrier。

第一版可以只实现显式 upload barrier；render graph 自动推导放到后续阶段。

## 8. 推荐接口形态

```cpp
class OMRendererDevice
{
  public:
    virtual auto beginFrame() -> OMRendererFrameContext * = 0;
    virtual auto createUploadBatch(OMRendererFrameContext *) -> OMRendererUploadBatch * = 0;
    virtual auto submitUploads(OMRendererUploadBatch *) -> OMRendererSyncPoint = 0;
};

class OMRendererFrameContext
{
  public:
    virtual auto uploadBatch() -> OMRendererUploadBatch * = 0;
    virtual auto graphicsSync() -> OMRendererSyncPoint = 0;
    virtual void retire(OMRendererObject *) = 0;
};

class OMRendererUploadBatch
{
  public:
    virtual void writeBuffer(OMRendererBuffer *, uint64_t offset, const void *, uint64_t size) = 0;
    virtual void copyBuffer(OMRendererBuffer *, OMRendererBuffer *, uint64_t offset, uint64_t length) = 0;
    virtual void writeTexture(OMRendererTexture *, const OMRendererTextureRegion &, const void *) = 0;
};
```

上层不再直接调用：

```cpp
buffer->updateData(...);
```

而是：

```cpp
frame->uploadBatch()->writeBuffer(buffer, 0, data, size);
```

由 frame 统一提交和同步。

## 9. 后端职责边界

| 能力 | 上层 | Vulkan | OpenGL | Metal |
|---|---|---|---|---|
| frame 生命周期 | 调用 | command buffer + fence | implicit + fence | command buffer |
| staging | 声明上传 | transfer queue | PBO ring | shared buffer |
| barrier | 声明 usage | pipeline barrier | glMemoryBarrier | hazard tracking |
| graphics 等待 upload | 传递 sync token | semaphore | fence / ordering | shared event |
| present | 调用 | swapchain | swap window | drawable present |
| resource retire | 调用 | frame fence | fence | command buffer completion |

## 10. 迁移步骤

1. 在 common 中定义 `FrameContext`、`UploadBatch`、`SyncPoint`、`ResourceLifetime` 接口；
2. 先把现有 Vulkan upload cmdbuff / semaphore / frame fence 收进 Vulkan 实现；
3. 把 OpenGL 的 buffer update、PBO、fence 收进 OpenGL 实现；
4. 删除 `bufferRequiresUpdate`、逐 buffer command submit 和逐 buffer `waitIdle`；
5. 将 `OMRendererBuffer::updateData` 标记为调试/同步回退路径，不再作为主要上传路径；
6. 增加 resource set 版本和 descriptor set 快照；
7. 最后用同一套接口实现 Metal 后端。

## 11. 最低验收清单

- [ ] 一帧只等待一次 frame fence；
- [ ] 一次 upload batch 最多一次 transfer submit；
- [ ] 上传路径中不存在逐 buffer `waitIdle`；
- [ ] 旧 task 不会因为 resource set 后续修改而看到新绑定；
- [ ] buffer 内容更新不要求重新 record task；
- [ ] 绑定目标变化要求重新 record task；
- [ ] resource set / descriptor set 在 frame 完成后回收；
- [ ] OpenGL、Vulkan 对同一上层操作的可见行为一致；
- [ ] resize / swapchain 重建时能安全清理 in-flight 上传和资源。
