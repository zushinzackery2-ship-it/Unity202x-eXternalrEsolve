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

---

> [!NOTE]
> **版本兼容性说明**  
> 本项目的结构与算法主要基于 Unity 2020~2022 版本进行逆向还原；Unity 版本过老时，部分结构偏移/算法可能无法完全兼容。  
> 本库仅支持 Windows x64（64 位）。

## 访问器

- **全局上下文**：通过 `SetGlobalMemoryAccessor` / `SetGOMGlobal` / `SetDefaultRuntime` 一次性设置，后续所有 API 直接调用、直接返回结果，无需反复传参。
- **可插拔内存访问**：默认提供 `WinAPIMemoryAccessor`（基于 `ReadProcessMemory`），可替换为自定义驱动实现 `IMemoryAccessor` 接口。
- **Header-only**：纯头文件库，无需编译，直接 `#include "ExternalResolve.hpp"` 即可使用。
- **Mono / IL2CPP 双支持**：通过 `RuntimeKind` 切换，自动适配两种运行时的内存布局差异。

## 特性

| 功能 | 说明 |
|:-----|:-----|
| **跨进程内存读取** | 基于 `ReadProcessMemory`，支持自定义驱动接口 |
| **GameObjectManager 遍历** | 枚举所有 GameObject / Component，支持按 Tag / Name 查找 |
| **Transform 世界坐标** | 通过 Hierarchy 层级累乘计算真实世界坐标 |
| **相机 W2S** | 读取相机矩阵，世界坐标转屏幕坐标 |
| **Metadata 导出** | 扫描并导出 IL2CPP `global-metadata`（返回二进制数据）；支持额外导出 `*.hint.json`（用于 dumper 侧生成 SDK）；`ExportMetadataTVersion`：若已设置 version 则精确匹配该版本；若未设置（version==0）则启用 header version 范围校验（[10,100]）通过即导出 |
| **Mono / IL2CPP** | 自动适配两种运行时的内存结构 |

> [!IMPORTANT]
> **GameObjectManager 偏移（可手动也可盲扫）**  
> GameObjectManager 全局指针偏移因游戏/Unity版本不同而异。  
> 该偏移指向 `gomGlobal`（全局指针槽地址）：`gomGlobal = unityPlayerBase + offset`。  
> 除手动逆向外，也可以在运行时盲扫 `UnityPlayer.dll` 的 `.data/.rdata` 指针槽自动定位（见 `FindGOMGlobalByScan`）。

---

## 编译要求

- **C++ 标准**：C++17 或以上
- **编译器**：MSVC (Visual Studio 2019+) 或 MinGW (GCC 9+)
- **平台**：Windows x64
- **头文件库**：GLM（数学库）

## 依赖安装

需要手动添加 [GLM 库](https://github.com/g-truc/glm)：

```bash
git clone https://github.com/g-truc/glm.git
```

目录结构：
```
项目根目录/
├── glm/
│   ├── glm.hpp
│   └── ...
├── ExternalResolve.hpp
├── Camera/
├── Core/
├── GameObjectManager/
├── Metadata/
├── MemoryAccessor/
└── Analysis/
```

---

<details>
<summary><strong>目录结构</strong></summary>

```
External/
├── Core/                  # 基础内存接口
│   ├── UnityExternalMemory.hpp        # IMemoryAccessor 接口
│   ├── UnityExternalMemoryConfig.hpp  # 全局访问器 + ReadPtrGlobal
│   └── UnityExternalTypes.hpp         # RuntimeKind / TypeInfo / GetManagedType
│
├── MemoryAccessor/        # 内存访问实现（可替换）
│   ├── Accessor/                      # 访问器实现
│   │   ├── DriverMemoryAccessor.hpp   # 驱动实现示例
│   │   └── WinAPIMemoryAccessor.hpp   # 默认 WinAPI 实现
│   ├── AccessorContainer.hpp          # 全局访问器容器
│   └── Resolver/
│       ├── UnityWindowResolver.hpp    # 按窗口类名枚举 PID
│       └── WinAPIResolver.hpp         # 进程/模块解析
│
├── GameObjectManager/     # GameObjectManager 遍历 + 原生结构
│   ├── GOMpUnit/
│   │   ├── GOM.hpp                    # 聚合头文件
│   │   ├── Bucket/                    # Hash Bucket 结构
│   │   │   ├── Bucket.hpp
│   │   │   └── HashCalc.hpp
│   │   ├── Globals/                   # 全局指针
│   │   │   └── GOMGlobal.hpp
│   │   ├── ListNode/                  # 链表节点
│   │   │   └── ListNode.hpp
│   │   ├── Managed/                   # 托管对象
│   │   │   └── Managed.hpp
│   │   ├── Manager/                   # 管理器
│   │   │   └── Manager.hpp
│   │   ├── Pool/                      # 对象池
│   │   │   └── Pool.hpp
│   │   ├── Search/                    # 查找功能
│   │   │   └── GOMSearch.hpp
│   │   └── Walker/                    # 遍历器
│   │       ├── GOMWalker.hpp
│   │       ├── WalkerImpl.hpp
│   │       └── WalkerTypes.hpp
│   ├── Managed/ManagedObject.hpp      # 托管对象封装
│   └── Native/
│       ├── NativeGameObject.hpp
│       ├── NativeComponent.hpp
│       └── NativeTransform.hpp
│
├── Camera/                # 相机 + W2S
│   ├── UnityExternalCamera.hpp        # FindMainCamera / GetCameraMatrix
│   └── UnityExternalWorldToScreen.hpp # WorldToScreenPoint
│
├── Metadata/              # IL2CPP global-metadata 扫描与导出
│   ├── Core/
│   │   └── Config.hpp                # 全局配置（目标进程/模块/版本）
│   ├── PE/
│   │   └── Parser.hpp                # PE 头/节区解析
│   ├── Header/
│   │   └── Parser.hpp                # Metadata 头部评分/大小计算
│   ├── Scanner/
│   │   ├── Pointer.hpp               # s_GlobalMetadata 指针扫描
│   │   └── Registration/
│   │       ├── Types.hpp             # Il2CppRegs, CodeRegOffsets
│   │       ├── Helpers.hpp           # 磁盘 PE 读取/范围检查
│   │       └── Scanner.hpp           # FindCodeRegistration/FindMetadataRegistration
│   ├── Hint/
│   │   ├── Struct.hpp                # MetadataHint 结构体
│   │   ├── Json.hpp                  # JSON 序列化
│   │   └── Export.hpp                # ExportMetadataHintJsonTScore/TVersion/ToFile
│   ├── Export/
│   │   └── Export.hpp                # ExportMetadataTScore/TVersion
│   └── MetadataAll.hpp               # 聚合头文件
│
├── Analysis/              # 算法与结构分析文档
│   ├── Structure/
│   │   ├── GOM数据结构.txt
│   │   ├── IL2CPP内存结构.txt
│   │   └── MONO内存结构.txt
│   └── Algorithm/
│       ├── GOM定位算法.txt
│       ├── GOM解析算法.txt
│       ├── Metadata检索与导出算法.txt
│       ├── TagHash掩码算法.txt
│       ├── Tag查找算法.txt
│       ├── Transform变换坐标算法.txt
│       └── 相机W2S算法.txt
│
└── ExternalResolve.hpp    # 统一入口（include 这个即可）
```

</details>

---

## 快速开始

```cpp
#include "External/ExternalResolve.hpp"

int main()
{
    // 1. 解析进程和模块（以 WinAPI 为例）
    UnityExternal::UseWinAPIResolvers();
    DWORD pid = UnityExternal::FindProcessId(L"Game.exe");
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid);

    // 2. 创建内存访问器并设置为全局
    UnityExternal::WinAPIMemoryAccessor accessor(hProcess);
    UnityExternal::SetGlobalMemoryAccessor(&accessor);

    // 3. 写入 gomGlobal 指针槽地址和默认运行时
    std::uintptr_t unityPlayerBase = UnityExternal::FindModuleBase(pid, L"UnityPlayer.dll");
    if (!unityPlayerBase)
    {
        std::cerr << "Failed to find UnityPlayer.dll" << std::endl;
        CloseHandle(hProcess);
        return 1;
    }
    
    UnityExternal::SetTargetPid(pid);
    std::uint64_t offset = UnityExternal::FindGOMGlobalByScan();
    if (!offset)
    {
        CloseHandle(hProcess);
        return 1;
    }
    UnityExternal::SetGOMGlobal(unityPlayerBase + (std::uintptr_t)offset);
    UnityExternal::SetDefaultRuntime(UnityExternal::RuntimeKind::Mono);
    // 或 RuntimeKind::Il2Cpp（根据目标游戏选择）

    // 4. 遍历所有 GameObject
    auto gameObjects = UnityExternal::EnumerateGameObjects();
    for (const auto& go : gameObjects)
    {
        try
        {
            UnityExternal::NativeGameObject nativeGO(go.nativeObject);
            std::string name = nativeGO.GetName();
            std::cout << "GameObject: " << name << std::endl;
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error reading GameObject: " << e.what() << std::endl;
        }
    }

    // 5. 查找主相机并读取相机矩阵
    std::uintptr_t camNative = UnityExternal::FindMainCamera();
    if (camNative)
    {
        glm::mat4 camMatrix = UnityExternal::GetCameraMatrix(camNative);

        // 6. 获取 Transform 世界坐标
        UnityExternal::Vector3f worldPos;
        UnityExternal::GetTransformWorldPosition(transformNative, worldPos);

        // 7. 世界坐标转屏幕坐标
        UnityExternal::ScreenRect screen{ 0, 0, 1920, 1080 };
        auto result = UnityExternal::WorldToScreenPoint(camMatrix, screen,
            glm::vec3(worldPos.x, worldPos.y, worldPos.z));

        if (result.visible)
        {
            // 绘制 ESP ...
        }
    }

    CloseHandle(hProcess);
    return 0;
}
```

---

## 自定义内存访问器

默认使用 `WinAPIMemoryAccessor`，可替换为驱动或其他实现：

```cpp
class MyDriverAccessor : public UnityExternal::IMemoryAccessor {
public:
    bool Read(std::uintptr_t address, void* buffer, std::size_t size) const override {
        return MyDriver::ReadMemory(address, buffer, size);
    }
    bool Write(std::uintptr_t address, const void* buffer, std::size_t size) const override {
        return MyDriver::WriteMemory(address, buffer, size);
    }
};

MyDriverAccessor accessor;
UnityExternal::SetGlobalMemoryAccessor(&accessor);
```

---

## 常见问题

### Q: 如何找到 GameObjectManager 全局指针偏移？

**A:**

- **手动逆向**：使用 IDA Pro 或 CheatEngine 动态分析 `Camera.get_main()` 等路径来定位。
 - **运行时盲扫**：使用 `FindGOMGlobalByScan()` 或示例工具 `gomScanner` 输出 `UnityPlayer.dll+0x????????`（表示 `gomGlobal` 槽的 RVA）。
   - `gomScanner` 会自动枚举 `UnityWndClass` 的 PID 并逐个尝试（无需手动传 pid）。
   - 扫描命中后会在当前目录输出 `global-metadata_pid_<pid>.dat` 与 `metadata_hint_tscore_pid_<pid>.json`。

### Q: 支持多进程同时读取吗？

**A:** 支持，通过 `IMemoryAccessor` 接口可并行读取多个进程。但需要为每个进程创建独立的 `accessor`，并通过参数传递而非全局访问器。

### Q: 是否支持多线程访问？

**A:** 支持，但内存访问本身是串行的（基于 `ReadProcessMemory`）。建议使用线程池模式进行批量查询。

---

<div align="center">

**Platform:** Windows x64 | **License:** MIT

</div>
