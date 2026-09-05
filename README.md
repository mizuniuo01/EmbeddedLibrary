# EmbeddedLibrary

面向 STM32、MSPM0、ESP32 等 MCU 项目的可复用 C11 组件库。公共组件保持硬件、BSP、
Driver 和具体业务无关，并优先在主机环境中验证。

## 当前状态

仓库正在按 [docs/PLAN.md](docs/PLAN.md) 建设。P0 已完成，P1 已包含基础契约、编码算法
及有界容器。第三批容器已完成本地验证，等待远端 CI；Services、BSP、Drivers 和 RTOS
适配尚未实现。组件契约见 `docs/components/`，阶段证据见 `docs/plans/P1_FOUNDATION.md`。

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
