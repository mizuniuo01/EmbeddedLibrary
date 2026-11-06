# Scheduler 组件说明

## 职责

`scheduler` 属于 Services 层，负责在裸机主循环中按固定优先级执行编译期确定的非阻塞周期任务。
它不管理硬件、DMA、ISR、任务栈、动态内存或 RTOS。

## 使用边界

Composition Root 提供单调 `uint32_t` tick，并在主循环中按以下顺序调用：

```text
读取项目时间值
    ↓
scheduler_update(now_tick)
    ↓
scheduler_run()
```

示例可以约定 `1 tick = 1 ms`，但组件不固定时间单位。任务回调必须非阻塞；任务需要检查 DMA 或外设数据时，读取项目侧 Driver/BSP 提供的数据状态。

## 调用方资源和装配

新项目通常由 Composition Root 或 Application 静态持有四类资源：

| 资源 | 所有者 | scheduler 的关系 | 生命周期要求 |
| --- | --- | --- | --- |
| `scheduler_t` | Composition Root/Application | 修改内部状态 | 覆盖整个调度器使用期 |
| `scheduler_slot_t[]` | Composition Root/Application | 借用并修改运行状态 | 覆盖整个调度器使用期 |
| `scheduler_task_config_t[]` | Composition Root/Application | 只读借用 | 覆盖整个调度器使用期且运行期不修改 |
| 每个任务的 `context` | 对应项目模块 | 回调时原样传回 | 覆盖该任务被调度的整个期间 |

组件不分配堆内存、不创建任务栈、不保存时间源，也不拥有 callback 或 `context` 指向的业务对象。

```c
static scheduler_t g_scheduler;
static scheduler_slot_t g_scheduler_slots[2];

static const scheduler_task_config_t g_scheduler_tasks[] = {
    {.callback = motor_task, .context = &g_motor, .priority = 0U, .period_ticks = 1U},
    {.callback = sensor_task, .context = &g_sensor, .priority = 1U, .period_ticks = 10U},
};

const mizu_status_t status =
    scheduler_init(&g_scheduler, g_scheduler_slots,
                   sizeof(g_scheduler_slots) / sizeof(g_scheduler_slots[0]),
                   g_scheduler_tasks, sizeof(g_scheduler_tasks) / sizeof(g_scheduler_tasks[0]));
```

`slots` 仅用于调用方静态分配。初始化会按优先级排序其物理元素，因此调用方不得直接读写槽成员，也不得把槽数组下标当作任务 ID；稳定任务 ID 始终是原配置表下标。

## 所有权与生命周期

- 配置表由调用方以 `const scheduler_task_config_t[]` 静态提供。
- 运行槽由调用方以 `scheduler_slot_t[]` 静态提供。
- 初始化时分别传入配置数量和槽数组容量，两者必须相等。
- scheduler 保存配置和槽数组指针，不复制配置、不分配、不释放。
- 调用方只提供槽数组存储，不得直接读写槽成员；配置表和槽数组必须覆盖整个调度器生命周期。
- `context` 由调用方拥有，必须在配置任务的整个生命周期内有效；scheduler 不复制或释放它。
- 初始化时按优先级和原配置索引排列运行槽；任务 ID 始终是原配置索引。
- 初始化后任务集合固定，不支持运行期增删或修改配置。
- 已确认设计为每个实例只允许一次成功初始化；再次初始化应被拒绝并保持旧实例完整可用。该约束当前尚待生产代码落实，见 `REVIEW-S1-001`。
- 统计通过快照 API 读取；槽数组公开只是为了静态分配，不构成可写公共状态接口。
- 初始化在项目装配阶段进行；更新、执行和查询在同一主循环上下文调用，不支持 ISR、并发调用或重入。

## 执行与时间语义

- 第一次 `scheduler_update()` 只建立时间基准。
- 周期单位与 `now_tick` 相同，周期必须大于 0 且不超过 `INT32_MAX`。
- 周期累计保留除完整周期外的余数。
- ready 任务每轮最多执行一次；额外完整周期记录为 `missed_count`，不追赶执行。
- 数值越小优先级越高；同优先级按任务 ID 升序执行。初始化排序为 O(N log N)，派发扫描为 O(N)，另加回调耗时。
- 回调返回错误时，继续执行其他 ready 任务，`scheduler_run()` 返回本轮首个错误。
- 调用方必须在同一主循环中先读取一次 tick 并调用 `scheduler_update()`，再调用 `scheduler_run()`；scheduler 不负责时间源的原子读取。
- 当相邻两次 `scheduler_update()` 的间隔超过 `INT32_MAX` 时，scheduler 丢弃该异常时间段、以当前 tick 重建基准并返回 `MIZU_STATUS_OUT_OF_RANGE`；下一次正常 update 可继续运行。是否记录、降级或进入安全状态由 Application/Domain 决定。

## 内部状态和调用方保证

每个槽维护 `elapsed_ticks`、`ready`、运行次数、错误次数、漏周期数和最近回调状态。首次 update 只建立时间基准；后续 update 累计周期，run 按优先级消费 ready 状态并调用回调。回调前 ready 已被清除，因此同一任务每轮最多执行一次。

调用方必须保证：

- 在同一主循环或已同步的普通任务上下文串行调用所有 API；
- 每轮先读取一次一致的单调 tick，再调用 `update()` 和 `run()`；
- callback 非阻塞、执行时间有界，且不重入 scheduler、不修改配置表或槽数组；
- 根据 `scheduler_run()` 和 `scheduler_update()` 的错误结果执行项目定义的诊断、降级或安全动作；
- 正常项目不使用空任务表；接口保留空配置能力仅用于通用性。

## 触发原因

`scheduler_trigger_t` 是稳定的触发原因接口；S1 只实现 `SCHEDULER_TRIGGER_PERIODIC`，不提供 ISR 通知、事件位或临界区端口。

## 验证

主机测试覆盖初始化、容量校验、周期推进、tick 回绕、异常重基准、跨任务优先级排序、错误继续执行、统计快照和计数饱和。

本地 GCC/Clang 完整检查、AddressSanitizer/UndefinedBehaviorSanitizer 测试和 Cortex-M0+ 编译已通过；目标板上的最坏执行时间和时基读取一致性仍由采用该组件的项目验证。
