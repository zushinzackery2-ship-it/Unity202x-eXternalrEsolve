# Task-Status

## 当前任务 / 待修清单

1. [ ] 空行膨胀 + 风格不一致（gom/、camera/、transform/、object/ 下文件）
2. [ ] reinterpret_cast 对齐 UB（4 处，应改 memcpy）
   - classmap.hpp:341 — `*reinterpret_cast<const uintptr_t*>(buf.data() + off)`
   - enumerate_objects.hpp:227 — `*reinterpret_cast<uintptr_t*>(objBuffer)`
   - enumerate_objects.hpp:239 — `*reinterpret_cast<uintptr_t*>(objBuffer + off.unity_object_managed_ptr)`
   - enumerate_objects.hpp:407 — `*reinterpret_cast<uintptr_t*>(objBuffer + off.game_object_name_ptr)`
3. [ ] classmap.hpp FindGameAssemblyDataSection 与 pe.hpp ReadModuleSections 逻辑重复 → 复用
4. [ ] validate_dlist.hpp 两处无上限循环需加 maxSteps 保护
   - :81 `while (true)` Floyd 环检测阶段
   - :111 `while (cycleCur != meet)` 测量环长阶段
   - :140 `for (;;)` 完整遍历验证阶段
5. [ ] gom_walker.hpp 逐指针 RPM 无批量优化（大场景性能瓶颈）
6. [ ] g_ctx / g_fieldOffsetState 全局状态需标注单线程约束注释
7. [ ] 纯算法函数缺离线单元测试

## 已完成

- 完整 Review 输出报告
- codeindex 核对确认全部待修项

## 经验 / 高价值发现

- 硬编码偏移（bones/camera/msid）是正确的，无需改动
- sdk_type_resolver.hpp 实际 397 行（15K 是字节），大小合理无需拆分
- FindGameAssemblyDataSection 手动解析 PE section，可用已有的 ReadModuleSections + .data 过滤替代
- ValidateCircularDList 用 Floyd 环检测，逻辑正确但缺 maxSteps 保护
