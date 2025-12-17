# UnityExternal 支持库函数清单

> 入口头文件：`ExternalResolve.hpp`（一次性包含全部组件）  
> 使用前需先设置全局内存访问器 `SetGlobalMemoryAccessor`，并写入 GameObjectManager 全局指针 `SetGOMGlobal` 与默认运行时 `SetDefaultRuntime`。

## Core

- `class IMemoryAccessor { bool Read(addr, buffer, size); bool Write(addr, buffer, size); }`：跨进程读写接口。
- `ReadValue<T>(mem, addr, out)` / `WriteValue<T>(mem, addr, value)`：模板读写。
- `ReadPtr(mem, addr, out)` / `ReadInt32(mem, addr, out)` / `ReadCString(mem, addr, out, maxLen)`：基础读取助手。
- `SetGlobalMemoryAccessor(const IMemoryAccessor*)` / `GetGlobalMemoryAccessor()`：设置/获取全局访问器。
- `ReadValueGlobal<T>(addr, out)` / `WriteValueGlobal<T>(addr, value)` / `ReadPtrGlobal(addr, out)` / `ReadInt32Global(addr, out)`：基于全局访问器的便捷读写。
- `enum class RuntimeKind { Il2Cpp, Mono };`
- `struct TypeInfo { std::string name; std::string namespaze; };`
- `GetManagedType(runtime, mem, managedObj, TypeInfo&)` / `GetManagedClassName(runtime, mem, managedObj, out)` / `GetManagedNamespace(runtime, mem, managedObj, out)`：托管对象类型信息。

## MemoryRead / WinAPI

- `using ProcessIdResolver` / `ModuleBaseResolver`：可插拔进程号/模块基址解析器。
- `SetProcessIdResolver(resolver)` / `SetModuleBaseResolver(resolver)`：注册自定义解析器。
- `UseWinAPIResolvers()`：使用内置 WinAPI 解析实现。
- `FindProcessId(processName)` / `FindModuleBase(pid, moduleName)`：调用当前解析器。
- `struct WinAPIMemoryAccessor : IMemoryAccessor`：基于 `ReadProcessMemory` 的默认跨进程访问器。

## GameObjectManager 全局配置

- `SetGOMGlobal(std::uintptr_t)` / `GetGOMGlobal()`：设置/获取 GameObjectManager 全局指针地址。
- `SetDefaultRuntime(RuntimeKind)` / `GetDefaultRuntime()`：设置/获取默认运行时（Mono/IL2CPP）。

## GameObjectManager Walker 与枚举

- `struct GameObjectEntry { node; nativeObject; managedObject; };`
- `struct ComponentEntry { nativeComponent; managedComponent; };`
- `class GOMWalker(mem, runtime)`：按当前访问器与运行时构造。
  - `EnumerateGameObjects(std::vector<GameObjectEntry>& out)`：遍历所有 GameObject。
  - `EnumerateComponents(std::vector<ComponentEntry>& out)`：遍历所有 Component（经由 GameObject 的组件池）。
- 全局便捷函数（使用全局访问器与默认运行时）：
  - `EnumerateGameObjects()` / `EnumerateComponents()`
  - `GetAllLinkedBuckets()`：返回所有 GameObject 桶的 bucket 头地址。
  - `FindGameObjectThroughTag(int32_t tag)`
  - `FindGameObjectThroughName(const std::string& name)`
  - `FindGameObjectWithComponent(const std::string& typeName)`
  - `GetComponentThroughTypeId(gameObjectNative, typeId)`
  - `GetComponentThroughTypeName(gameObjectNative, typeName)`

## Bucket 辅助函数

- `GetBucketHashmask(bucket, outMask)`：读 bucket 第 0~4 字节的 hashMask。
- `GetBucketInfo(bucket, outPrefix)`：读 bucket 第 4~8 字节作为 4 字符前缀。
- `GetBucketHashValue(bucket, outValuePtr)`：读 bucket 最后 8 字节的 value 指针。
- `FindBucketThroughHashmask(hashMask)`：按 hashMask 在 GameObjectManager 中查找 bucket，返回 bucket 地址。

## 托管对象封装

- `struct ManagedObject { runtime; address; }`
  - `IsValid()`
  - `GetNative(std::uintptr_t& outNative)`：managed +0x10 → native 指针。
  - `GetTypeInfo(TypeInfo& out)` / `GetClassName()` / `GetNamespace()`

## 原生 GameObject / Component

- `struct NativeGameObject { address; }`
  - `IsValid()`
  - `GetManaged(out)` (+0x28)
  - `GetComponentPool(out)` (+0x30)
  - `GetComponentCount(out)` (+0x40)
  - `GetTag(out)` (+0x54，返回低 16 位逻辑 TagId)
  - `GetComponentTypeIds(std::vector<int32_t>& out)`
  - `GetName()` (+0x60 → C 字符串)
  - `GetComponent(index, outNativeComponent)`：组件池读取。

- `struct NativeComponent { address; }`
  - `IsValid()`
  - `GetManaged(out)` (+0x28)
  - `GetGameObject(out)` (+0x30)
  - `NativeComponent_GetGameObject(nativeComponent, out)`：同上辅助。

## Transform 与世界坐标

- `struct NativeTransform { address; }`
  - `IsValid()`
  - `GetManaged(out)` (+0x28)
  - `GetGameObject(out)` (+0x30)
  - `ReadHierarchyState(TransformHierarchyState& state, int32_t& index)`
  - `GetWorldPosition(Vector3f& outPos, int maxDepth = 256)`：层级累乘计算世界坐标。
- 辅助函数：
  - `ComputeWorldPositionFromHierarchy(state, index, outPos, maxDepth)`
  - `ReadTransformHierarchyState(transformAddr, state, index)`
  - `GetTransformWorldPosition(transformAddr, outPos, maxDepth)`
  - `FindTransformOnGameObject(gameObjectNative, outTransformNative)`
  - `FindTransformOnGameObject(runtime, gameObjectNative, outTransformNative)`（进阶：自指定运行时）

## 标签 Hash 辅助

- `CalHashmaskThrougTag(tagId)` / `CalTagThroughHashmask(tagId)`：根据 TagId 计算底层 `core::base_hash_map` 使用的 4 字节哈希掩码。

## 相机与 W2S

- `IsComponentEnabled(nativeComponent)`：检查组件启用标志 (+0x38)。
- `GetCameraMatrix(nativeCamera)`：读取视图投影矩阵 (Camera +0x100) 为 `glm::mat4`。
  - 注意：该函数依赖当前实现假定的 Camera 内部布局与偏移（默认 `+0x100`）。部分游戏/Unity 版本/渲染管线下该偏移可能无效或矩阵语义不同，表现为 W2S 不稳定/全部不可见/坐标漂移。此时需要自行适配矩阵来源与偏移（或改用你已确认正确的 view/proj/viewProj 读取路径）。
- `FindMainCamera()`：按 Tag=5 找主相机并返回原生 Camera 指针（需组件启用）。

- 结构：
  - `struct ScreenRect { x, y, width, height; }`
  - `struct WorldToScreenResult { visible; x; y; depth; }`
- 函数：
  - `WorldToScreenPoint(viewProj, screen, worldPos)`：简化版，返回屏幕坐标与可见性。
  - `WorldToScreenPointFull(viewProj, camPos, camForward, screen, worldPos)`：附带深度计算。

## Metadata (IL2CPP global-metadata)

- 全局配置：
  - `SetMetadataTargetModule(std::uintptr_t moduleBase)` / `GetMetadataTargetModule()`
  - `SetMetadataTarget(DWORD pid, const wchar_t* moduleName)`：默认模块名 `GameAssembly.dll`
  - `SetMetadataTargetVersion(uint32_t version)` / `GetMetadataTargetVersion()`

- 导出函数（返回二进制数据，调用者自行写文件）：
  - `ExportMetadataTScore()`：评分模式定位并导出
  - `ExportMetadataTVersion()`：版本模式定位并导出
    - version 由 `SetMetadataTargetVersion` 提供
    - 若未设置（version==0），则按版本区间 [10,100] 做严格扫描（strictVersion=true）
      仍找不到则建议改用 `ExportMetadataTScore()`

- Hint JSON 导出函数（返回 JSON 字符串或直接写文件）：
  - `ExportMetadataHintJsonTScore(DWORD pid = 0, const wchar_t* processName = nullptr, const wchar_t* moduleName = nullptr)`
  - `ExportMetadataHintJsonTVersion(DWORD pid = 0, const wchar_t* processName = nullptr, const wchar_t* moduleName = nullptr)`
  - `ExportMetadataHintJsonTScoreToFile(const std::filesystem::path& outPath, DWORD pid = 0, const wchar_t* processName = nullptr, const wchar_t* moduleName = nullptr)`
  - `ExportMetadataHintJsonTVersionToFile(const std::filesystem::path& outPath, DWORD pid = 0, const wchar_t* processName = nullptr, const wchar_t* moduleName = nullptr)`
  - `ExportMetadataHintJsonTScoreToSidecar(const std::filesystem::path& metadataDatPath, DWORD pid = 0, const wchar_t* processName = nullptr, const wchar_t* moduleName = nullptr)`：输出到 `metadataDatPath + ".hint.json"`

- 兼容别名（转发到新函数名）：
  - `ExportMetadataThroughScore()` -> `ExportMetadataTScore()`
  - `ExportMetadataThroughVersion()` -> `ExportMetadataTVersion()`
