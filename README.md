# EmbeddedLibrary

面向 STM32、MSPM0、ESP32 等 MCU 项目的可复用 C11 组件库。公共组件保持硬件、BSP、
Driver 和具体业务无关，并优先在主机环境中验证。

## 当前状态

仓库已完成原 P0-P3 的基础设施、基础组件、控制算法和并发实验组件，成熟度为
`host-tested / hardware-unverified`。当前按新的 [总体建设计划](docs/PLAN.md) 推进：主线聚焦最小裸机调度、
数据交换、通信、诊断、参数和菜单；成熟能力按实际需求引入，RTOS 项目优先使用内核原生机制。
现有代码的规划定位见 [已有资产登记](docs/ASSET_REGISTER.md)，组件契约和历史阶段证据继续保存在
`docs/components/` 与 `docs/plans/`。

## 主机检查

当前主机验证基线为 Linux/WSL，需要 CMake 3.20 或更新版本、Ninja、GCC 13、Clang 21、
clang-format 21、clang-tidy 21、ripgrep 和 cppcheck：

```sh
cmake --preset host-gcc
cmake --build --preset host-gcc
ctest --preset host-gcc
sh tools/check.sh host-gcc
sh tools/check.sh host-clang
cmake --preset host-sanitize
cmake --build --preset host-sanitize
ctest --preset host-sanitize
```

可用 Preset：`host-gcc`、`host-clang`、`host-sanitize`。其中 GCC 和 Clang Preset 分别调用
`gcc-13` 和 `clang-21`；Sanitizer Preset 同时启用 ASan 和 UBSan。构建产物位于 `build/`，
不会提交。Native Windows 和 macOS 当前不在持续验证矩阵内。

架构、代码风格和仓库工作流分别见 `docs/ARCHITECTURE_STANDARD.md`、
`docs/CODING_STYLE.md` 和 `docs/REPOSITORY_WORKFLOW.md`。
