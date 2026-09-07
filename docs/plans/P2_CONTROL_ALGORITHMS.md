# P2 控制与算法库阶段设计

状态：已完成（三批实现、本地验证和远端 CI 均已闭环）

## 批次

1. PID：当前实现 f32，显式 dt、双导数模式、条件积分/回算抗饱和、显式 reset。
2. 滤波与非线性：五个独立 Libraries，分别实现一阶滤波、固定窗口滑动平均、迟滞、死区、速率限制。
3. 组合控制器：只组合已存在的控制器实例，不复制算法逻辑。

定点算法暂不纳入 P2；后续另行设计。所有批次均不包含 RTOS、调度、BSP、Driver、执行器输出或产品语义。

## 第一批 PID 契约

- 配置和输入必须有限；输出上下限、积分上下限为闭区间，且下限不得大于上限。
- `dt` 必须为有限正数；失败不修改 PID 历史状态或输出参数。
- reset 清除积分并记录当前测量值和误差；首次 calculate 使用零历史差分。
- 导数可选择测量值或误差；条件积分在饱和继续推动时保持旧积分；回算使用 tracking gain。
- 实例由调用方静态分配，组件不使用堆、VLA、时钟、RTOS 或全局可变状态。

## 验收

主机 GCC/Clang/Sanitizer、ARM compile-only、clang-format、Clang-Tidy、Cppcheck 和外部接入测试均通过；
测试覆盖正常、饱和、reset、异常输入、失败原子性及多实例隔离后，才完成第一批。

第二批当前证据：五个独立组件均具备公共头文件、实现、独立 CMake 目标、单元测试和组件文档。
GCC 18 项 CTest、Clang 18 项 CTest、Sanitizer 19 项 CTest（含 sanitizer probe）、外部接入测试、
Clang-Tidy、Cppcheck 和 ARM Cortex-M0+ compile-only 均已在本地通过。第二批提交
`968b0588aeebd10e5f9a968ba9b3f10827e0dbe9` 对应运行 `34087853238` 的五项远端任务全部成功。

第三批采用固定两级串级 PID、两个 `pid_f32_t` 指针、组合器级间限幅、PID 受控快照/恢复和事务式回滚。
组合器输入为外环目标、外环测量、内环测量和显式 dt；保存最近一次成功的中间目标与最终输出。

第三批当前证据：新增 `cascaded_pid` 独立 Library、PID snapshot/restore API、19 项 GCC/Clang
仓库测试、20 项 Sanitizer 测试、外部 CMake 接入、Clang-Tidy、Cppcheck 和 ARM compile-only
均已在本地通过。提交 `3b9efb6e6714690712ed6f77b64ec8b28ef10d2a` 对应运行
[`34128638025`](https://github.com/mizuniuo01/EmbeddedLibrary/actions/runs/34128638025) 的五项远端任务全部成功。

## P2 收尾

P2 三批均已完成公共 API、独立构建目标、组件说明、主机单元测试、外部接入验证和 ARM compile-only。
PID snapshot/restore 只作为组合器事务回滚的受控接口；串级控制器固定为两级，组合器不包含产品领域语义。
本阶段不提供定点算法、RTOS 调度、目标硬件验证、目标 WCET 或执行器输出，成熟度为
`host-tested / hardware-unverified`。

下一阶段为 P3 并发端口与 FreeRTOS 适配，仍需用户手动开启阶段性 Plan；P3 不在本次收尾中自动实现。
