<div align="center">

# er2 — Unity 运行时结构还原框架

**跨进程运行时内省框架 · Unity 2020–2023 (Windows x64)**

*Header-only C++17 · 无需符号表/RTTI · Mono & IL2CPP 自动识别*

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey?style=flat-square)
![Header Only](https://img.shields.io/badge/Header--Only-Yes-green?style=flat-square)

</div>

---

## 项目概述

纯算法库，仅凭跨进程只读内存访问，从运行中的 Unity 进程中还原引擎运行时数据结构——不依赖符号表、RTTI、文件系统或代码注入。

> [!NOTE]
> **版本兼容性**  
> 面向 Unity 2020–2023（Windows x64），自动识别 Mono / IL2CPP 后端。  
> Unity 6 版本请移步：[Unity6-eXternalrEsolve](https://github.com/zushinzackery2-ship-it/Unity6-eXternalrEsolve)

---

## 部分实现原理

### GameObjectManager 盲结构发现

GOM 非导出符号，通过**多级指针链扫描**定位：

1. **Seed** — 全地址空间模式匹配，找到堆上满足桶步长 + 对齐约束的候选
2. **表头推导** — 从 seed 反向回溯哈希表基址
3. **两级全局解析** — 堆 → `.data`/`.rdata` → 全局槽，逐级收窄 + 候选评分

候选验证基于结构性约束（Floyd 环检测、桶一致性、指针范围），不使用字节签名。

> [!TIP]
> 完整扫描链耗时约 5–7s（seed ~2.5s → scan1 ~3s → scan2 ~2ms），scan2 利用 PE 段解析约束在 `.data`/`.rdata` 范围内。

### IL2CPP 类型系统远程重建

从进程内存纯只读重建类型信息表：

- 评分式启发定位 metadata blob（不读文件）→ 构建 `byval_arg → 类名` 映射
- 以 `System.Object` / `System.String` 等已知类型名为锚点，暴力定位 `Il2CppClass*[]` 指针表
- 遍历 `metadataRegistration → fieldOffsetsTable` 链解析字段偏移

### 逆向还原的结构

| 结构 | 字段 / 偏移 |
|:-----|:-----------|
| **GomManager** | +0x00 buckets_ptr, +0x08 bucket_count, +0x28 local_list_head |
| **GomBucket** | stride 0x18: +0x00 hashmask, +0x04 info, +0x08 key, +0x10 list_head |
| **GomListNode** | +0x00 prev, +0x08 next, +0x10 nativeObject |
| **NativeGameObject** | +0x28 managed, +0x30 componentPool, +0x40 componentCount, +0x54 tag, +0x60 name_ptr |
| **NativeComponent** | +0x28 managedComponent, +0x30 gameObject, +0x38 enabled |
| **ComponentPool** | slot stride 0x10: +0x00 typeId, +0x08 nativeComponent |
| **NativeObject (基类)** | +0x08 instanceID, +0x28 managed_ptr |
| **NativeScriptableObject** | +0x38 name_ptr |
| **MsIdToPointerSet** | +0x00 entriesBase, +0x08 capacity, +0x0C count |
| **MsIdToPointerEntry** | stride 0x18: +0x00 hashMask, +0x08 key (instanceID), +0x10 object* |
| **NativeTransform** | +0x38 hierarchyState_ptr, +0x40 index |
| **TransformHierarchyState** | +0x18 nodeData, +0x20 parentIndices; node stride 0x30 (localPos + quat + scale) |
| **NativeCamera** | +0x100 viewProjMatrix (64 bytes, 4×4 float) |

### 分层架构

```
┌─────────────────────────────────────────┐
│           应用层 / 测试用例              │
├─────────────────────────────────────────┤
│        init/* （薄封装便利层）            │  ← 全局上下文封装
├──────┬──────┬───────┬───────┬───────────┤
│ gom/ │ msid/│object/│camera/│ metadata/ │  ← 领域算法层
│      │      │       │transf/│ dumpsdk/  │
├──────┴──────┴───────┴───────┴───────────┤
│    IMemoryAccessor （纯接口）            │  ← 平台抽象边界
├─────────────────────────────────────────┤
│    os/win/ （WinAPI 实现）               │  ← 可替换后端
└─────────────────────────────────────────┘
```

- **零耦合** — 算法层仅依赖 `const IMemoryAccessor&`（单方法读接口）
- **头文件为主** — 领域算法在 `include/er2/`；DumpSDK 离线 collect / Sidecar / DummyDll 另有可编译源 `src/dumpsdk/`
- **后端可替换** — `IContextBackend` 抽象进程句柄、模块枚举和内存访问；默认提供 `WinApiContextBackend`
- **进程内访问** — `LocalMemoryAccessor`（`VirtualQuery` + SEH），供注入/同进程调用 metadata、registration 扫描与 `DumpSdkRunInProcess`

---

## 完整 API 参考

> [!IMPORTANT]
> **调用前提**  
> 调用 `er2::AutoInit()` 成功后，以下所有 API 可直接使用，无需手动传递 `mem` / `offsets` 等参数。  
> **返回值约定**：返回 `std::optional<T>` 的函数成功时有值、失败时为空；返回 `bool` + out 参数的函数成功返回 `true`。

### 上下文与初始化

| API | 说明 |
|:----|:-----|
| `AutoInit()` | 自动发现 Unity 进程，执行 GOM/MSID 扫描，填充全局上下文 |
| `IsInited()` | 是否已完成初始化 |
| `Pid()` | 返回目标进程 PID |
| `Runtime()` | 返回运行时类型 `ManagedBackend::Mono` / `ManagedBackend::Il2Cpp` |
| `UnityPlayerBase()` | 返回 `UnityPlayer.dll` 模块基址 |
| `SetContextBackend(backend)` | 切换上下文后端（默认 `WinApiContextBackend`） |
| `Mem()` | 返回内存访问器 `const IMemoryAccessor&` |
| `Off()` / `GomOff()` / `CamOff()` / `TransformOff()` | 返回各模块偏移量结构体 |
| `GomGlobalSlotVa()` / `MsIdToPointerSlotVa()` | 返回 GOM / MSID 全局槽虚拟地址 |
| `ReadPtr(addr)` | 读取指针，返回 `optional<uintptr_t>` |
| `ReadValue<T>(addr)` | 读取任意类型值，返回 `optional<T>` |

### 结构发现（GOM）

| API | 说明 |
|:----|:-----|
| `GomManager()` | 返回 GameObjectManager 地址 |
| `GomBucketsPtr()` | 返回哈希桶表基址 |
| `GomBucketCount()` | 返回桶数量 |
| `GomLocalGameObjectListHead()` | 返回本地 GameObject 链表头 |
| `CheckGomManagerCandidate()` | 对候选地址执行结构验证，返回 `{ok, score}` |
| `EnumerateGameObjects()` | 遍历所有 GameObject，返回 `optional<vector<GameObjectEntry>>` |
| `GetGameObjectByName(name)` | 按名称精确查找，返回 `vector<uintptr_t>` |
| `FindGameObjectThroughTag(tag)` | 按 Tag 查找第一个 GameObject |
| `GetListNodeNative(node)` | 读取链表节点的原生对象指针 |
| `GetListNodeNext(node)` | 读取链表节点的下一个节点 |

### 类型系统（IL2CPP）

| API | 说明 |
|:----|:-----|
| `EnsureIl2CppTypeInfoInited()` | 初始化并校验 `typeInfoTable`（含 metadata 导出 + 类型表定位） |
| `FindClassIndex(fullName)` | 通过完整类型名获取 byval 索引（如 `"UnityEngine.Transform"` → `idx`）|
| `FindClassByIndex(idx)` | 通过 byval 索引获取 `Il2CppClass*` 地址 |
| `FindClass(fullName)` | 通过完整类型名直接获取 `Il2CppClass*` 地址 |

### 字段偏移（IL2CPP）

| API | 说明 |
|:----|:-----|
| `SupportsDynamicFieldOffsets()` | 当前运行时是否支持 metadata 字段偏移解析（Mono=false，IL2CPP=true） |
| `EnsureFieldOffsetsInited()` | 初始化动态字段偏移缓存 |
| `TryGetFieldOffset(type, field, out)` | 查询指定类型的字段偏移，如 `("UnityEngine.Object", "m_CachedPtr", out)` |
| `GetFieldOffsetOr(type, field, fallback)` | 查询失败时返回 fallback 值 |

### 对象内省

| API | 说明 |
|:----|:-----|
| `GetGameObjectName(nativeGo)` | 读取 GameObject 名称（bool 和 optional 两种重载） |
| `GetScriptableObjectName(nativeObj)` | 读取 ScriptableObject 名称 |
| `GetManagedObjectTypeInfo(managedObj)` | 获取托管对象的命名空间 + 类名 |
| `FindObjectsOfTypeAll(className)` | 按类名在全局注册表中枚举所有实例 |
| `FindObjectsOfTypeAll(ns, className)` | 按命名空间 + 类名过滤枚举 |
| `EnumerateMsIdToPointerObjects(opts, callback)` | 遍历 MSID 注册表，支持过滤 GameObject / ScriptableObject |
| `MsIdSetPtr()` / `MsIdCount()` | MSID 集合基址与计数 |

### 组件查询

| API | 说明 |
|:----|:-----|
| `GetAllComponents(go)` | 获取 GameObject 的所有组件，返回 `vector<uintptr_t>` |
| `GetComponentThroughTypeId(go, typeId)` | 按 TypeId 获取特定组件 |
| `GetComponentThroughTypeName(go, typeName)` | 按类型名获取特定组件 |
| `GetTransformComponent(go)` | 获取 Transform 组件（快捷方式） |
| `GetCameraComponent(go)` | 获取 Camera 组件（快捷方式） |

### 空间数据

| API | 说明 |
|:----|:-----|
| `GetTransformWorldPosition(transform)` | 从层级变换链解析世界坐标（bool 和 optional 两种重载） |
| `GetBoneTransformAll(rootGo)` | 遍历 Transform 子树获取骨骼列表 `vector<BoneTransformAllItem>` |
| `FindMainCamera()` | 查找主相机的原生组件地址 |
| `GetCameraMatrix(cam)` | 获取相机视图投影矩阵（bool 和 optional 两种重载） |
| `WorldToScreenPoint(viewProj, screen, pos)` | 世界坐标 → 屏幕坐标变换 |

### DumpSDK（进程内 / 跨进程）

| API | 说明 |
|:----|:-----|
| `DumpSdkRunInProcess(mem, gaBase, gaSize, outDir, result)` | 进程内完整产物：Collect → Sidecar → DummyDll（+ 可选 `generic.json`） |
| `DumpSdkDump(mem, moduleBase, moduleSize, outDir, paths, …)` | 写出 `dump.cs` / `generic.json` / `global-metadata.dat` / hint |
| `DumpSdk6DumpByPid(pid, paths)` | 跨进程薄包装（`OpenProcess` + `DumpSdkDump`） |

产物目录典型文件：`dump.cs`、`il2cpp.h`、`script.json`、`stringliteral.json`、`generic.json`、`global-metadata.dat`、`il2cpp-offline.hint.json`、`DummyDll/*.dll`。

---

## 编译要求

| 项目 | 要求 |
|:-----|:-----|
| **C++ 标准** | C++17 |
| **编译器** | MSVC（Visual Studio 2022） |
| **平台** | Windows x64 |
| **运行时** | 静态链接（`/MT`） |
| **第三方库** | [GLM](https://github.com/g-truc/glm)（矩阵运算） |

---

## 测试覆盖

> [!NOTE]
> **测试结果**  
> 冒烟测试覆盖完整 API 调用链：上下文初始化 → 结构发现 → 对象遍历 → 类型系统重建 → 组件查询 → 空间数据提取 → 托管对象内省 → 字段偏移交叉验证。

| 测试套件 | 覆盖范围 | 状态 |
|:---------|:---------|:-----|
| `tests/winapi_smoke` | 全量 API 冒烟测试（对接运行中的 Unity 进程） | Mono: **33 PASS** · IL2CPP: **46 PASS** · **0 FAIL** |
| `tests/unit` | 纯算法单元测试（哈希、投影变换、四元数旋转） | 全部通过 |

---

<details>
<summary><strong>目录结构</strong></summary>

```
er2/
├── Resolve202x.hpp                 # 统一入口头文件
├── include/er2/
│   ├── compat/                     # 编译器/平台兼容适配
│   ├── core/                       # 基础类型定义
│   ├── mem/                        # IMemoryAccessor 纯接口
│   ├── os/win/                     # Windows 平台后端实现
│   └── unity2/
│       ├── camera/                 # 投影矩阵提取与坐标变换
│       ├── core/                   # 结构偏移量定义
│       ├── dumpsdk/                # SDK 导出 + offline/writers 头
│       ├── gom/                    # 对象管理器扫描与遍历
│       ├── init/                   # 全局上下文薄封装层
│       ├── metadata/               # IL2CPP 元数据重建
│       │   ├── header/             #   头部解析与验证
│       │   ├── registration/       #   Registration 表扫描
│       │   ├── hint/               #   启发式 Hint 系统
│       │   └── codegen/            #   CodeGen 支持
│       ├── msid/                   # 实例 ID 全局注册表
│       ├── object/                 # 对象内省
│       │   ├── managed/            #   托管 (.NET) 对象访问
│       │   └── native/             #   原生 (C++) 对象访问
│       ├── transform/              # Transform 层级与世界坐标
│       └── util/                   # 共享工具函数
├── src/dumpsdk/                    # DumpSDK 可编译源（offline + writers）
└── tests/
    ├── unit/                       # 算法单元测试
    └── winapi_smoke/               # 集成冒烟测试
```

</details>

---

## 快速开始

```cpp
#include "Resolve202x.hpp"
#include <cstdio>

int main()
{
    if (!er2::AutoInit())
    {
        return 1;
    }

    std::printf("Runtime: %s\n",
        er2::Runtime() == er2::ManagedBackend::Il2Cpp ? "IL2CPP" : "Mono");

    // 枚举指定类型的所有实例
    auto transforms = er2::FindObjectsOfTypeAll("UnityEngine", "Transform");
    std::printf("Transform 实例数: %zu\n", transforms.size());

    // IL2CPP: 从 metadata 解析字段偏移（无硬编码）
    if (er2::SupportsDynamicFieldOffsets())
    {
        std::uint32_t offset = 0;
        if (er2::TryGetFieldOffset("UnityEngine.Object", "m_CachedPtr", offset))
        {
            std::printf("m_CachedPtr offset = 0x%X\n", offset);
        }
    }

    // IL2CPP: 按完整类型名查询 Il2CppClass*
    const std::uintptr_t klass = er2::FindClass("UnityEngine.Transform");
    if (klass)
    {
        std::printf("Transform class @ 0x%llX\n", (unsigned long long)klass);
    }

    return 0;
}
```

---

<div align="center">

**Platform:** Windows x64 &nbsp;|&nbsp; **License:** MIT

</div>

> [!CAUTION]
> **免责声明**  
> 本项目仅用于 Unity 引擎内部结构的学术研究、合法授权的 Modding/插件开发学习与逆向工程研究。  
> 使用者须自行确保遵守相关法律法规与服务条款。



