# mizu_status_service 组件说明

`mizu_status_service` 属于 Services 层，只保存上层显式选择的少量错误状态，供 OLED、通信或日志等顶层出口查询。它不自动接收所有组件状态，也不执行错误处理。

## 职责和调用模型

- 下层组件继续通过 `mizu_status_t` 返回状态；直接调用方负责重试、停止、降级或其他处理。
- 只有上层判断需要顶层输出时，才调用 `report(source_id, status)`。
- 服务不扫描模块、不订阅普通状态、不触发恢复回调。
- report/clear/query 由单一主循环或普通任务上下文串行调用；服务不提供锁、ISR 安全或 RTOS 同步。

## 存储

- 记录数组由 Composition Root/Application 静态提供；`source_id == 0` 表示空槽。
- 服务只借用记录数组，不分配、不释放、不创建隐藏全局存储；记录数组必须在服务生命周期内保持有效，且不得与服务对象重叠。
- 每个来源最多保存一条当前错误记录；重复上报更新原记录。
- 新来源使用首个空槽；容量耗尽返回 `MIZU_STATUS_FULL`，不覆盖其他来源。
- `clear(source_id)` 显式释放来源槽位，后续可复用。
- 记录查询复制 `source_id` 和 `status`，不消费记录、不暴露内部数组指针。

## 约束

- `source_id == 0` 非法。
- `report()` 不接受 `MIZU_STATUS_OK`；恢复使用 `clear()`。
- 服务不保存时间、严重级别、业务载荷或错误历史。
