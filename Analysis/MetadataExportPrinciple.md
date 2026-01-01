# global-metadata 导出原理（只讲算法）

 这里说的“metadata”指 IL2CPP 的 **global-metadata**（Unity 运行时会把它加载到进程内存里）。导出本质上就是：

 - 你能读目标进程内存（不管你用什么方式）
 - 选定一个要扫描的模块（通常是 `GameAssembly.dll`）
 - 在这个模块的某些 section 里扫描“像指针”的 8 字节值
 - 把这些候选指针当作“可能指向 metadata header”，对 header 做一堆结构约束校验 + 打分
 - 选分最高的那个当 metadata 基址，然后根据 header 计算总大小，把整段 bytes 读出来

---

 ## 1. 你需要提供什么（必要输入）

 - **能读目标进程内存**
   - 最低要求：给定一个地址和长度，能读出 bytes。
   - 注意，如果使用驱动，Cr3方式会面临非常多的问题。
 - **要扫描的模块基址**
   - 通常选 `GameAssembly.dll`（IL2CPP 的 metadata 通常可由它的全局指针关联到）。
 - **可选：版本约束**
   - 版本为 `0` 表示“不指定固定版本”。
   - 如果你知道目标 Unity/IL2CPP 的 metadata 版本，可以把它当作硬约束，用来减少误命中。

---

 ## 2. 前置条件（不满足就必失败）

 - **读取权限/稳定性**：你对目标进程的内存读取要稳定。
   - 过程中任何一次读失败，都可能导致“导出为空”。
 - **模块基址必须正确**：你要扫描的模块基址如果错了，后续所有 PE 解析/section 扫描都会错。
 - **目标确实是 IL2CPP**：Mono 项目没有 `global-metadata` 这个对象（至少不是同一套导出思路）。

---

 ## 3. 整体流程（从 0 到拿到 metadata bytes）

 1. **解析目标模块的 PE 头**
    - 得到 `SizeOfImage`，以及 section 列表（名字、RVA、大小）。
 2. **选取要扫描的 section**
    - 只扫描更可能出现“全局指针槽/指针值”的 section（见下一节）。
 3. **在这些 section 里按 8 字节步进扫候选指针**
    - 把每个 8 字节当成一个 `uint64`，先做“像不像用户态指针”的廉价过滤。
 4. **对候选指针指向的内存做 header 结构校验 + 打分**
    - 找到分数最高的那个，认为它指向 metadata header。
 5. **用 header 推导 metadata 总大小**
    - 通过一组 offset/size 字段计算 `maxEnd`，作为总大小。
 6. **把 `[metaBase, metaBase+totalSize)` 整段读出来**
    - 分块读（比如 1MB 一块），拼成最终 bytes。

---

 ## 4. 关键点：怎么定位 metadata header（`metaBase`）

 可以把它理解成“从模块里捞出一堆可能是指针的值，然后用一套规则判断哪个指针最像 metadata header”。

 ### 4.1 解析模块 PE section（在远程进程里读）

 - 读取 DOS/NT 头与 section 表，拿到每个 section 的：
   - 名字（`.data/.rdata/...`）
   - RVA
   - VirtualSize（必要时用 RawSize 兜一下）
 - 当前实现只支持 x64 模块。

 ### 4.2 为什么只扫这些 section

 扫描范围会刻意收窄到更可能出现“全局指针槽/指针值”的区域，比如：

 - `.data`
 - `.rdata`
 - `.pdata`
 - `.tls`
 - `.reloc`

 直观原因：全局变量/指针槽/只读数据更集中在这些段里。

 ### 4.3 扫指针：按 8 字节步进 + 低成本过滤

 - section 会按 chunk 分块读取（比如 2MB 一块）。
 - chunk 内按 8 字节步进，把每个 `uint64` 当成“候选指针值”。
 - 对候选做廉价过滤（减少误报、加速）：
   - 不为 0
   - 落在典型的 x64 用户态地址范围内
   - 8 字节对齐
 - 过滤后的候选按“所在 page”分组（`ptr & ~0xFFF`），下一步会读取对应的 4KB page 来验证。

 ### 4.4 header 校验/打分到底在查什么

 对每个候选指针指向的位置，会在 page 内做一套“像不像 metadata header”的校验与打分。核心思路是：

 - **对齐/最小长度**：header 起点必须 4 字节对齐，且在当前 page 里至少能覆盖到一段固定长度。
 - **版本字段**：
   - 如果你指定了“必须是某个版本”，就要求它严格相等。
   - 如果你没指定版本，但开启了更严格模式，会要求版本在一个合理范围内（避免离谱误命中）。
 - **关键表的 offset/size 自洽**（重点）：
   - 字符串表 offset/size 必须非 0，offset 对齐且不小于某个最小值。
   - images/assemblies 这类表必须非 0，且 size 必须能整除固定的“单元素结构大小”（用来推算 count）。
   - 多组 offset/size 成对字段共同决定一个 `maxEnd = max(offset + size)`。
   - 要求 `maxEnd` 落在合理区间，且 strings/images/assemblies 都不能越过 `maxEnd`.

 本质上不是找签名，而是用“很多字段同时满足逻辑约束”来排除假 header。

 ### 4.5 快速可读性验证

 拿到“分数最高的候选”后，会做一个很便宜的 sanity check：

 - 尝试读取 `metaBase + maxEnd - 1` 的 1 个字节

 目的就是：避免选到一个“看起来像 header，但后面根本不可读”的假指针。

---

 ## 5. 找到 metaBase 后：怎么算 totalSize、怎么整段读出

 ### 5.1 totalSize 怎么算

 读取 header 里的多组 offset/size 对，计算：

 - `totalSize = max(offset + size)`

 也就是：header 里最大那张表的结尾位置，作为整个 metadata blob 的总大小。

 ### 5.2 怎么把整段读出来

 - 按固定 chunk 大小循环读取（比如 1MB 一块），拼接成最终 bytes。
 - 任何一次读取失败都直接判失败（返回空），避免得到“残缺数据”。

 最终得到的就是：目标进程内存里的 `global-metadata` 原始 bytes（从 header 开始整段）。

---

 ## 6. 两种工作模式（宽松 vs 带版本约束）

 这套导出一般会提供两种思路，本质差异在于“版本字段怎么用”：

 - **宽松模式**
   - 不强制版本范围，主要靠结构自洽 + 打分来选 best。
   - 优点：适配面更广。
   - 缺点：在强保护/读不稳定/噪声很大时，误命中的概率更高.

 - **带版本约束的模式**
   - 如果你知道确切版本：直接要求必须匹配该版本。
   - 如果你不知道确切版本：也可以启用“版本必须在合理区间”的约束，减少离谱误报.
   - 优点：更稳.
   - 缺点：版本填错会直接扫不到.

---

 ## 7. Hint JSON 是啥（为什么要额外导出它）

 纯导出 bytes 只能拿到数据本体；Hint JSON 记录的是“你是怎么定位到这段数据的”，用于复现/排错/离线分析.

 典型会记录：

 - 进程信息：pid、进程名（可选）
 - 模块信息：模块名、模块路径（可选）、模块基址、模块大小、PE ImageBase（可选）
 - metadata 定位信息：
   - **指针槽地址**（哪个地址存着那根指针）
   - **metadata header 基址**（那根指针最终指向哪里）
   - `total_size`、`magic`、`version`、images/assemblies 的 count（能算出来就写）
 - IL2CPP registrations（能扫到就写）：
   - codeRegistration / metadataRegistration
   - 以及它们相对模块基址的 RVA

---

 ## 8. 为什么会导出为空（按概率从高到低）

 - **内存读取不稳定/被拦截**：读某个 chunk/page 失败，流程直接判失败.
 - **模块基址错了**：PE 头/section 表读出来不对，后面全部跑偏.
 - **误命中/噪声太大**：宽松模式下打分选错候选，后续 totalSize 计算或整段读取失败.
 - **版本约束不匹配**：你填了版本，但目标不是这个版本，会直接扫不到.
 - **目标不是 IL2CPP**：Mono 场景下这套逻辑本来就不成立.

---

 ## 9. 测试程序里做了什么（你看到的输出对应到哪一步）

 测试程序的行为可以概括成：

 1. 找到目标进程里 `GameAssembly.dll` 的基址
 2. 把“目标模块基址”配置好
 3. 运行上面第 3~5 节的扫描与导出流程，得到 metadata bytes
 4. 把 bytes 落盘
 5. 额外生成一份 Hint JSON（记录定位信息）并落盘

