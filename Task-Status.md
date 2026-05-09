# Task-Status

## 待办

- [ ] scan1 全进程扫描(~3s)，后续可缩窄到 MEM_PRIVATE 页
- [ ] ScriptableObjectName 偏移兼容性（0x38 对部分 Unity 版本无效，低优先级）

## 已完成

- Smoke test 全覆盖：Mono 33 PASS/6 SKIP，IL2CPP 46 PASS/4 SKIP，0 FAIL
- IL2CPP klassmap 验证：FindClassIndex/FindClass/FindClassByIndex + m_CachedPtr cross-validate 通过
- GOM new_chain 优化：25s→5.4s（seed→表头→scan1→scan2）
- 代码质量：UB 修复、PE 解析复用、格式化(-2317行)、单元测试通过
- 已 push 到 Re-Arch 分支

## 经验

- EnumerateGameObjects 首条目可能 name 不可读（local list 的 GO 没有 name），fallback 需遍历尝试
- IL2CPP m_CachedPtr offset=0x10 是稳定的，可用于 native↔managed 互验
- FindMainCamera 需要 Camera component，tag=5 GO 存在但没 Camera 时返回 0（场景依赖）
