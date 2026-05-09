# Task-Status

## 当前任务 / 待修清单

（无）

## 已完成

- 完整 Review 输出报告
- codeindex 核对确认全部待修项
- reinterpret_cast UB → memcpy（classmap.hpp + enumerate_objects.hpp 4处）
- FindGameAssemblyDataSection 复用 ReadModuleSections 消除重复 PE 解析
- validate_dlist.hpp maxSteps 已回滚（大场景 20w+ GO 会被误杀）
- gom_walker.hpp 节点读取合并为单次 24 字节 RPM（3→2 RPM/node）
- g_ctx / g_fieldOffsetState 加单线程约束注释
- 25 个文件格式化去除空行膨胀（总计 -2317 行 / +227 行）
- 纯算法单元测试（hash / W2S / QuatRotateSIMD）全部通过
- 已 push 到 Re-Arch 分支
- AutoInit GOM 扫描优化：level1 改为 MEM_PRIVATE 可写页指针对齐扫描并命中即验证；level2 改为只扫 UnityPlayer `.data/.rdata`

## 经验 / 高价值发现

- 硬编码偏移（bones/camera/msid）是正确的，无需改动
- sdk_type_resolver.hpp 实际 397 行（15K 是字节），大小合理无需拆分
- FindGameAssemblyDataSection 手动解析 PE section，可用已有的 ReadModuleSections + .data 过滤替代
- ValidateCircularDList 用 Floyd 环检测，逻辑正确；硬编码 maxSteps 会误杀 20w+ GO 大场景，已回滚
