# waiter

`waiter` 是 queue、event group、count notification 和 mutex 共用的等待者状态对象。调用方为每个
同时进行的等待提供独立静态工作区；一个 waiter 只能绑定一个 owner 对象，完成或取消后必须显式
reset 才能再次使用。`waiter_port_t` 提供 `wait`、`signal_one`、`signal_all` 和 `cancel_all`，对象不包含
pthread、FreeRTOS 或任务句柄。事件组广播后由各 waiter 重新检查条件，核心不保存跨线程 waiter 指针。
P3 已通过 POSIX queue/event/count notification 等待测试和 TSan 验证；FreeRTOS 适配器及目标硬件运行
验证留待首个实际项目。
