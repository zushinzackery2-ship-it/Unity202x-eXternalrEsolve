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

er2 是一个**纯算法库**，能够从运行中的 Unity 进程内存中**还原引擎运行时数据结构**——不依赖调试符号、RTTI 或源代码。

仅凭一个进程句柄和原始内存读取原语，er2 可以完成：

- **发现与验证**引擎内部的对象管理结构（哈希分桶双向循环链表）
- **重建 IL2CPP 类型系统**，从进程内存中的 metadata 解析类层次和字段偏移
- **穿越 Managed↔Native 对象桥接**，在 .NET 托管对象与 C++ 原生对象之间建立映射
- **解析层级空间数据**（Transform 树、投影矩阵）从不透明的二进制内存布局中提取

项目的核心贡献是一套**启发式扫描与结构验证算法**，使得在完全不了解目标二进制符号表的前提下完成上述工作。

> [!NOTE]
> **版本兼容性**  
> 面向 Unity 2020–2023（Windows x64），自动识别 Mono / IL2CPP 后端。  
> Unity 6 版本请移步：[Unity6-eXternalrEsolve](https://github.com/zushinzackery2-ship-it/Unity6-eXternalrEsolve)

---

## 技术亮点

### 盲扫指针链算法

引擎的核心对象管理器通过**多级指针链扫描**定位，无任何硬编码地址：

1. **Seed 扫描** — 全地址空间模式匹配，找到堆上包含已知结构特征（桶步长 + 对齐）的对象
2. **表头推导** — 从 seed 反向回溯，定位哈希表基址
3. **两级全局解析** — 级联模式扫描从 堆 → `.data` 段 → 全局槽 逐级收窄，每级带候选评分

每个候选地址通过**结构性检查**验证（循环双向链表完整性使用 **Floyd 龟兔赛跑环检测**、桶一致性、指针范围启发式），而非签名匹配，使方法对编译器/版本变化具有鲁棒性。

> [!TIP]
> **性能数据**  
> 在典型场景中，完整的 GOM 扫描链耗时约 5–7 秒（seed_scan ~2.5s → scan1 ~3s → scan2 ~2ms），其中 scan2 利用 PE 段解析将搜索范围限制在 UnityPlayer 模块的 `.data`/`.rdata` 段内，将全进程扫描降至毫秒级。

### 远程 IL2CPP 类型系统重建

对于 IL2CPP 构建，er2 完全从进程内存重建类型信息表：

- 使用**评分式启发算法**从远程内存中导出并解析 `global-metadata.dat` 头部（不假设文件路径）
- 从 metadata 字节流构建 `byval_arg → 类名` 映射表
- 通过采样已知类型并验证名称往返一致性，定位 `Il2CppClass*[]` 指针表
- 遍历 `metadataRegistration → fieldOffsets` 链解析**运行时字段偏移**，实现无符号的结构体字段访问

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
- **纯头文件** — 所有逻辑在 `include/er2/` 下的 `.hpp` 文件中，无链接时依赖
- **后端可替换** — `IContextBackend` 抽象进程句柄、模块枚举和内存访问；默认提供 `WinApiContextBackend`

---

## 核心算法与数据结构

| 算法 | 源文件 | 用途 |
|:-----|:-------|:-----|
| **多级指针链扫描** | `gom/scan_chain.hpp` | 无符号环境下从原始内存定位引擎全局单例 |
| **Floyd 龟兔赛跑环检测** | `gom/validate_dlist.hpp` | 验证不可信内存中的循环双向链表完整性 |
| **启发式结构评分** | `gom/manager_score.hpp` | 按结构一致性（链表完整性、桶对齐、指针范围）对候选地址排名 |
| **PE 节区解析** | `metadata/pe.hpp` | 识别 `.data`/`.rdata` 节以约束扫描范围 |
| **Metadata 头部评分定位** | `metadata/export.hpp` | 通过统计性头部字段验证定位 IL2CPP metadata blob |
| **锚点验证型类型表暴力搜索** | `init/classmap.hpp` | 以已知类型名为锚点采样验证，定位 `Il2CppClass*[]` 表 |
| **四元数 → 世界坐标 SIMD 管线** | `transform/` | 从层级局部变换重建世界坐标 |
| **三维投影（视图投影矩阵）** | `camera/` | 提取相机矩阵并执行坐标空间变换 |

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
│       ├── dumpsdk/                # SDK / 类型描述符导出
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



