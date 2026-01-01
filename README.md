<div align="center">

# ExternalResolve202x

**Unity 2020-2022 运行时内存结构算法还原库（er2 / header-only）**

*跨进程读取 | Header-Only | IL2CPP/MONO*

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey?style=flat-square)
![Header Only](https://img.shields.io/badge/Header--Only-Yes-green?style=flat-square)

</div>

---

> [!CAUTION]
> **免责声明**  
> 本项目仅用于学习研究 Unity 引擎内部结构与算法还原，以及在合法授权前提下的游戏 Modding/插件开发学习与验证，不得用于任何违反游戏服务条款或法律法规的行为。  
> 使用本项目产生的一切后果由使用者自行承担，作者不承担任何责任。  
> 请在合法合规的前提下使用本项目。

> [!NOTE]
> **版本兼容性说明**  
> 本项目面向 Unity 2020~2023（Windows x64）。对比 Unity6 版本在结构偏移与布局上存在差异。  
> 支持 Mono 与 IL2CPP 两种后端（AutoInit 自动识别）。   
> Unity6版本请移步至仓库:https://github.com/zushinzackery2-ship-it/Unity6-eXternalrEsolve   

> [!IMPORTANT]
> **代码重构说明**  
> 当前项目包含'**相当大量**'的AI重构代码，可能存在大量维护性问题。  
> 功能实现集中在 `include/er2`，入口侧尽量保持薄封装。

## 访问器

- **最小内存访问抽象**：以 `IMemoryAccessor` 为核心，算法层只依赖读内存接口。
- **Windows 适配**：默认提供 WinAPI 实现（`ReadProcessMemory`）适配器。
- **Header-only**：纯头文件库。

## 功能概览

| 功能 | 说明 |
|:-----|:-----|
| **AutoInit + 上下文管理** | 自动发现 Unity 进程、定位 UnityPlayer/GameAssembly (IL2CPP) 或 Mono 模块、缓存关键 offset/全局槽，并暴露 `g_ctx + Mem()` 读写入口 |
| **跨进程内存访问** | 以 `IMemoryAccessor` 为核心，提供 WinAPI 适配，所有算法都只依赖统一的读内存接口 |
| **GOM 扫描与遍历** | 盲扫 GameObjectManager、校验桶结构、枚举 GameObject/组件，支持按 tag/名称/组件类型快速搜索 |
| **MSID 全局注册表** | 枚举 `UnityEngine.Object` 实例，筛选 GameObject/ScriptableObject，读取名称、InstanceID、托管类型等信息 |
| **Transform / Camera / W2S** | 解析 Transform 层级得到世界坐标；读取相机视图投影矩阵并完成世界坐标到屏幕坐标转换 |
| **Bones** | 遍历 Transform 子树获取骨骼索引、名称与世界坐标 |
| **IL2CPP Metadata + Hint 导出** | 自动扫描 metadata header、导出 `global-metadata.dat`，并可生成 `*.hint.json` |
| **DumpSDK2 工具链** | 结合 metadata/hint 结果生成 C# API 描述与泛型结构信息，辅助离线分析/SDK 导出 |
| **模块化 Header-only 设计** | `er2/unity2/*` 下分门别类的子模块（gom/msid/object/camera/transform/metadata 等），可按需引用 |

---

## 核心 API 列表

| 模块 | 代表 API | 说明 |
|:----|:---------|:-----|
| **上下文 / AutoInit** | `AutoInit()` / `IsInited()` | 自动发现 Unity 进程、刷新 `g_ctx` |
|  | `ReadPtr(addr)` / `ReadValue<T>(addr)` | 基于 `g_ctx + Mem()` 的统一读内存封装 |
| **GOM** | `GomManager()` / `GomBucketsPtr()` / `FindGameObjectThroughTag(tag)` | 访问 GameObjectManager、遍历桶并按 tag 搜索 |
|  | `EnumerateGameObjects()` / `GetGameObjectByName(name)` | 列举 GameObject，或按名称精确查找 |
| **MSID** | `MsIdToPointerSlotVa()` / `MsIdCount()` | 读取 ms_id_to_pointer set 元数据 |
|  | `FindObjectsOfTypeAll(ns, name)` | 枚举或按命名空间+类型名查找 `UnityEngine.Object` 实例 |
| **对象/名称** | `ReadGameObjectName(nativeGo)` | 读取 Native/Managed 对象名称 |
| **Transform / Camera / W2S** | `GetTransformWorldPosition(transformPtr)` | 解析层级状态，输出世界坐标 |
|  | `FindMainCamera()` / `GetCameraMatrix(nativeCamera)` | 找主相机、读取视图投影矩阵 |
| **Metadata / Hint** | `ExportGameAssemblyMetadataByScore()` | 一次性导出 metadata bytes 与 hint json |
| **DumpSDK2** | `DumpSdk` 相关接口 | 生成 C# API 与泛型结构描述 |

---

## AutoInit 后可直接调用的 API

调用 `er2::AutoInit()` 成功后，以下 API 可直接使用，无需手动传递 mem/offsets 等参数。

| 分类 | API | 说明 |
|:-----|:----|:-----|
| **上下文** | `Pid()` | 返回目标进程 PID |
|  | `IsInited()` | 是否已初始化 |
|  | `Runtime()` | 返回 Backend 类型 (IL2CPP/Mono) |
| **GameObject** | `EnumerateGameObjects()` | 枚举所有 GameObject，返回 `optional<vector<GameObjectEntry>>` |
|  | `FindGameObjectThroughTag(tag)` | 按 Tag 查找 GameObject |
|  | `GetGameObjectByName(name)` | 按名称查找 GameObject |
| **组件** | `GetTransformComponent(go)` | 获取 Transform 组件 |
|  | `GetCameraComponent(go)` | 获取 Camera 组件 |
| **Transform** | `GetTransformWorldPosition(transform)` | 获取 Transform 世界坐标 |
|  | `GetBoneTransformAll(rootGo)` | 获取骨骼列表 |
| **相机 / W2S** | `FindMainCamera()` | 查找主相机 |
|  | `GetCameraMatrix(cam)` | 获取相机 ViewProj 矩阵 |
|  | `WorldToScreenPoint(viewProj, screen, pos)` | 世界坐标转屏幕坐标 |
| **MSID** | `FindObjectsOfTypeAll(className)` | 按类名查找所有实例 |
| **Metadata** | `ExportGameAssemblyMetadataByScore()` | 导出 metadata 字节 |

> **返回值说明**：返回 `optional<T>` 的函数成功时有值，失败时为空；返回 `bool` + out 参数的函数成功返回 true。

---

## 编译要求

- **C++ 标准**：C++17
- **编译器**：MSVC（Visual Studio 2022）
- **平台**：Windows x64
- **链接方式**：静态运行时（/MT）
- **第三方库**：仓库已包含 `glm/`（工具侧编译 bat 已默认添加 include）

---

<details>
<summary><strong>目录结构</strong></summary>

```
External-oldVersion4Unity202x/
├── Resolve202x.hpp             # 统一入口头文件
├── include/
│   └── er2/
│       ├── core/               # 基础类型定义
│       ├── mem/                # 内存访问抽象
│       ├── os/win/             # Windows 平台实现
│       └── unity2/
│           ├── camera/         # 相机与 W2S
│           ├── core/           # 偏移定义
│           ├── dumpsdk/        # SDK 导出
│           ├── gom/            # GameObjectManager
│           ├── init/           # AutoInit 封装层
│           ├── metadata/       # IL2CPP 元数据
│           ├── msid/           # InstanceID 映射
│           ├── object/         # Native/Managed 对象
│           ├── transform/      # Transform 世界坐标
│           └── util/           # 工具函数
├── Analysis/
│   ├── Algorithms/             # 算法说明文档
│   └── Structures/             # 结构说明文档
├── tools/                      # 示例工具
└── docs/                       # 补充文档
```

</details>

---

## 快速开始

```cpp
#include "Resolve202x.hpp"
#include <cstdio>

int main()
{
    // 自动发现 Unity 进程并初始化上下文
    if (!er2::AutoInit())
    {
        return 1;
    }

    // 输出环境信息
    printf("Runtime: %s\n", er2::Runtime() == er2::ManagedBackend::Il2Cpp ? "IL2CPP" : "Mono");

    // 查找主相机
    const std::uintptr_t cam = er2::FindMainCamera();
    if (cam)
    {
        // 获取相机矩阵
        auto viewProjOpt = er2::GetCameraMatrix(cam);
        if (viewProjOpt.has_value())
        {
            // ...
        }
    }

    // 按名称查找游戏对象
    auto controller = er2::GetGameObjectByName("Controller");
    if (controller) 
    {
        printf("Controller: 0x%llX\n", (unsigned long long)controller);
    }

    return 0;
}
```

---

## AutoInit 与 init/* 封装

- `er2::AutoInit()` 会自动定位 Unity 进程并填充全局上下文 `g_ctx`
- `include/er2/unity2/init/*` 提供基于 `g_ctx + Mem()` 的薄封装，尽量减少手动传参


