# EmbeddedLibrary

面向 STM32、MSPM0、ESP32 等 MCU 项目的可复用 C11 组件库。公共组件保持硬件、BSP、
Driver 和具体业务无关，并优先在主机环境中验证。

## 当前状态

S1 裸机调度器已完成本地验证。仓库中已有的旧生产代码作为历史资产保留；新的生产文件将随着学习和实际项目逐步积累。总体路线见
[docs/PLAN.md](docs/PLAN.md)，生产文件清单见 [docs/FILE_CATALOG.md](docs/FILE_CATALOG.md)。
规范和 Profile 是代码建设的前置依据；具体阶段由用户手动开启，未开启阶段不创建生产文件。

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

可用 Preset：`host-gcc`、`host-clang`、`host-sanitize`、`host-tsan`、
`arm-cortex-m0plus-gcc`。其中 GCC 和 Clang Preset 分别调用
`gcc-13` 和 `clang-21`；Sanitizer Preset 同时启用 ASan 和 UBSan。构建产物位于 `build/`，
不会提交。Native Windows 和 macOS 当前不在持续验证矩阵内。

检查脚本对当前阶段源码清单 `tools/checked_sources.txt` 执行格式和静态分析；旧资产仍参与构建和已有测试。
后续阶段完成新组件后，再把对应文件加入该清单。

架构、代码风格和仓库工作流分别见 `docs/ARCHITECTURE_STANDARD.md`、
`docs/CODING_STYLE.md` 和 `docs/REPOSITORY_WORKFLOW.md`。
