# 嵌入式 C/C++ 风格规范说明

> 版本：v1.0.0<br>
> 状态：Active<br>
> 适用范围：采用 `docs/ENGINEERING_PROFILE.yaml` 的 MCU 裸机/RTOS 项目

本文档是风格规范的解释入口，不复制完整的外部标准。`ENGINEERING_PROFILE.yaml` 是 Agent skills 使用的机器可读规则和项目上下文来源；本文档解释规则、来源和边界。Profile 的显式配置优先，未覆盖的内容继续采用其声明的外部标准。

## 1. 外部规范来源

### C

- C 规则采用：[BARR-C:2018](https://barrgroup.com/embedded-c-coding-standard)。
- C 标准由 Profile 的 `language.c.standard` 固定，默认 C11。
- 厂商生成代码、CMSIS、HAL、SDK 和第三方源码遵循其权威边界，不由本项目规则强行重排。

### C++

- C++ 嵌入式规则采用：[BARR-C:2018](https://barrgroup.com/embedded-c-coding-standard) 的嵌入式缺陷预防原则。
- 风格参考：[Google C++ Style Guide](https://google.github.io/styleguide/cppguide)。
- 语言和设计参考：[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)。
- C++ 必须按嵌入式 Profile 使用。默认 C++17；异常、RTTI、动态分配、标准库、协程和 C++20 特性必须由 Profile 明确启用并验证。
- C++ 命名默认采用 Profile 中的 `personal_rules.naming.cpp`（Google C++ Style）；C 的模块前缀命名规则不自动套用到 C++ 类型和函数。
- 指针声明排版按语言分别采用 Profile 的 `personal_rules.naming.pointer_declaration`；C 沿用 Linux 风格，C++ 遵循 Google C++ Style。
- ISR、硬实时路径、任务入口、DMA、MMIO、栈、堆、Flash、ABI 和启动初始化属于额外嵌入式边界。

## 2. 个人规则

当前确认的个人风格规则包括：

- 普通代码、宏和行尾注释的目标列宽为 100 列。
- C 函数、变量和文件使用 `snake_case`；公共符号使用模块前缀；类型使用 `<module>_<role>_t`；宏使用 `UPPER_SNAKE_CASE`。
- 模块前缀允许缩写，前提是缩写稳定可理解、不冲突、在模块范围内一致，并在首次使用时定义；缩写不得删除区分职责所需的字符。
- 函数名与左括号之间不加空格；控制语句关键字与左括号之间加空格。
- 混合逻辑条件中的比较项保留额外括号；复杂表达式用括号明确求值顺序。
- 二元运算符两侧留空格；一元运算符、`.`、`->` 和数组下标按 Linux/BARR-C 规则排版。
- 控制语句采用 Linux/K&R 同行开括号；函数定义开括号另起一行。
- 单语句控制流允许省略花括号；多语句分支必须使用花括号。
- `switch` 的 `case` 比 `switch` 多缩进一层。
- 编辑器按 Tab 键操作但插入 4 个空格；文件不保存 Tab 字符，Tab 宽度为 4。
- 长参数列表按 Linux 风格对齐到左括号；列宽仍受 100 列约束。
- 函数默认不超过 100 行；超出时必须有复杂度或职责例外依据。
- 函数采用单一出口优先；多资源清理按反向顺序在统一清理路径处理。
- 一行只声明一个对象；变量使用最小作用域，并在语义允许时声明即初始化。
- 宏和预处理器采用 BARR-C 安全规则，并采用 Linux 的可读性限制。
- Include 采用自有头文件优先、头文件自洽、禁止依赖传递包含。
- 类型和转换采用 BARR-C 主规则与 Linux 可读性原则；普通指针不得通过 typedef 隐藏，不透明句柄允许使用 typedef。
- 中文普通注释采用 Linux 注释思想；公共 API 在需要生成接口文档或契约不明显时使用中文 kernel-doc 风格，其他函数不强制使用 Doxygen。
- 许可证由项目管理；源文件保留模块职责、接口约束或维护原因等必要说明。

个人规则只记录与外部规范不同的明确选择。未写入 Profile 的内容继续采用所引用外部规范，不由 agent 推测个人偏好。

## 3. Profile 使用方式

项目规范配置文件为 `docs/ENGINEERING_PROFILE.yaml`；该文件声明语言、外部规范、C++ 嵌入式限制、执行模型、工具链、ABI、个人规则和结构化覆盖。Profile 中的 `overrides` 是外部规范差异的唯一登记位置。

没有 Profile 时，agent 必须报告“规范来源未确定”，不得宣称代码符合本规范。

## 4. Agent 执行约定

agent 生成或审查代码时，先读取 Profile，判断文件语言、目标平台和执行模型，再读取本文件与 Profile 引用的外部规范。输出必须区分外部规范要求、个人规则、项目事实和未验证项目；发现冲突时列出来源和影响。

## 5. 规则边界

外部规范负责完整的命名、控制流、类型、类、模板、标准库和注释规则；本节只记录已经确认的项目差异。项目个人偏好必须写入 Profile，而不是散落在 agent 对话中。

## Changelog

### v1.0.0 — 2026-10-10

- 将 C/C++ 风格规范收敛为外部标准索引、嵌入式边界和个人规则入口。
- 增加 Profile 驱动和 agent 执行约定。
