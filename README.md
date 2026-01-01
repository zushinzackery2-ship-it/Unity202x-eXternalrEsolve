<div align="center">

# UnityExternalResolve

**Unity 运行时内存结构算法还原库**

*跨进程读取 | Header-Only | Mono & IL2CPP*

![C++](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey?style=flat-square)
![Header Only](https://img.shields.io/badge/Header--Only-Yes-green?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)

</div>

---

> [!CAUTION]
> **免责声明**  
> 本项目仅用于学习研究 Unity 引擎内部结构与算法还原，以及在合法授权前提下的游戏 Modding/插件开发学习与验证，不得用于任何违反游戏服务条款或法律法规的行为。  
> 使用本项目产生的一切后果由使用者自行承担，作者不承担任何责任。  
> 请在合法合规的前提下使用本项目。

> [!NOTE]
> **版本兼容性说明**  
> 本项目的结构与算法主要基于 Unity 2020~2022 版本进行逆向还原；Unity 版本过老时，部分结构偏移/算法可能无法完全兼容。  
> 本库仅支持 Windows x64（64 位）。

> [!IMPORTANT]
> **代码重构说明**  
> 当前项目包含大量 AI 重构代码，如发现问题欢迎提交 Issue。

## 访问器

- **全局上下文**：通过 `er2::AutoInit()` 自动初始化上下文，后续所有 API 可直接调用，无需反复传参。
- **可插拔内存访问**：默认提供 `WinApiMemoryAccessor`（基于 `ReadProcessMemory`），也可以替换为你自己的 `IMemoryAccessor` 实现。
- **Header-only**：纯头文件库，无需编译，直接 `#include "Resolve202x.hpp"` 即可使用。
- **Mono / IL2CPP 双支持**：`AutoInit()` 自动识别运行时；也可用 `InitSettings(pid, ManagedBackend)` 手动指定。

## 特性

| 功能 | 说明 |
|:-----|:-----|
| **进程/模块解析** | 提供 `FindProcessId` / `FindModuleBase` 等接口（解析策略可替换） |
| **跨进程内存访问** | 默认 `WinApiMemoryAccessor`（`ReadProcessMemory`），也可自定义 `IMemoryAccessor` |
| **全局上下文配置** | `AutoInit` / `InitSettings` / `InitBase` 等一次性配置 |
| **Mono / IL2CPP** | 通过 `ManagedBackend` 区分运行时（AutoInit 自动识别） |
| **GameObjectManager 定位** | `AutoInit()` 内部会对 `UnityPlayer.dll` 做盲扫定位并初始化 |
| **GameObject / Component 枚举** | `EnumerateGameObjects` / `EnumerateComponents` 遍历对象 |
| **按 Tag/Name/类型查找** | 支持按 Tag、Name、脚本类型等条件查找（见 `include/er2/unity2/gom/*`） |
| **Transform 世界坐标** | 读取层级并做矩阵/向量运算，得到真实世界坐标 |
| **相机 W2S** | 读取相机矩阵，世界坐标转屏幕坐标 |
| **IL2CPP Metadata 扫描/导出** | 扫描并导出 `global-metadata`，并可额外导出 `*.hint.json` |

---

## 编译要求

- **C++ 标准**：C++17 或以上
- **编译器**：MSVC (Visual Studio 2019+) 或 MinGW (GCC 9+)
- **平台**：Windows x64
- **头文件库**：GLM（数学库）

## 依赖安装

项目已内置 `glm/` 目录，无需额外安装。

---

<details>
<summary><strong>目录结构</strong></summary>

```
项目根目录/
├── include/
│   └── er2/
│       ├── core/
│       ├── mem/
│       ├── os/
│       │   └── win/
│       └── unity2/
│           ├── core/
│           ├── gom/
│           ├── object/
│           ├── camera/
│           ├── transform/
│           ├── msid/
│           ├── metadata/
│           ├── dumpsdk/
│           └── init/
├── glm/
├── imgui/
├── Analysis/
├── docs/
├── tools/
├── Resolve202x.hpp
├── test.cpp
└── build_test.bat
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

    auto controller = er2::GetGameObjectByName("Controller");
    printf("Controller: 0x%llX\n", controller);

    return 0;
}
```

---

## 常见问题

### Q: 如何找到 GameObjectManager 全局指针偏移？

**A:**

- **手动逆向**：使用 IDA Pro 或 CheatEngine 动态分析 `Camera.get_main()` 等路径来定位。
- **运行时盲扫**：直接调用 `er2::AutoInit()`，内部会对 `UnityPlayer.dll` 做盲扫定位并初始化上下文。

### Q: 支持多进程同时读取吗？

**A:** 支持，通过 `IMemoryAccessor` 接口可并行读取多个进程。但需要为每个进程创建独立的 `accessor`，并通过参数传递而非全局访问器。

### Q: 是否支持多线程访问？

**A:** 支持，但内存访问本身是串行的（基于 `ReadProcessMemory`）。建议使用线程池模式进行批量查询。

---

<div align="center">

**Platform:** Windows x64 | **License:** MIT

</div>
