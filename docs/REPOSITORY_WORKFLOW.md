# 嵌入式工程仓库、Git 与 GitHub 工作流指南

> 版本：v1.0.0<br>
> 创建日期：2026-07-24<br>
> 发布日期：2026-07-24<br>
> 最后更新：2026-07-24<br>
> GitHub 功能复核日期：2026-07-24<br>
> 状态：Active<br>
> 适用范围：MCU 裸机、RTOS、厂商代码生成工程，以及包含 Python 工具、视觉模型或上位机辅助程序的混合仓库

本文是一份工程实践指南，负责解释专业嵌入式工程如何组织仓库、使用 Git，并通过 GitHub 完成协作与交付。
内容从个人开发所需的基础能力开始，逐步延伸到 Pull Request、持续集成、分支保护和正式发布。

本文提供推荐路径、选择依据、操作步骤和风险说明。项目规模、人员数量、交付责任和现有工具链共同决定最终做法；
专业性来自边界清楚、过程可追溯、结果可复现，以及投入与风险相匹配。

嵌入式 C 的命名、排版、注释和语言规则见 [CODING_STYLE.md](CODING_STYLE.md)；软件分层、模块边界、
运行模型和设计模式见 [ARCHITECTURE_STANDARD.md](ARCHITECTURE_STANDARD.md)。本文只解释物理仓库如何承载这些内容，
不重复定义逻辑架构职责。

---

## 1. 阅读路径与基本概念

### 1.1 建议阅读顺序

第一次建立完整工作流时，按以下顺序学习：

1. 阅读第 2～5 章，建立仓库文件分类和目录设计能力。
2. 在临时目录跟随第 6～13 章练习 Git，不使用重要工程作为实验对象。
3. 阅读第 13～17 章，完成一次从 Issue 到 PR 合并的 GitHub 练习。
4. 项目需要自动构建或正式发布时，再启用第 18～20 章的能力。
5. 日常开发使用第 21～23 章的场景流程、检查清单和命令速查。

有经验的读者可以按主题查阅。Git 命令示例默认在仓库根目录执行；包含占位符的命令必须替换后使用。

### 1.2 三种逐步增长的工程场景

| 场景 | 典型情况 | 合理起点 |
| --- | --- | --- |
| 个人开发 | 学习、比赛、原型、个人长期项目 | 清晰目录、可复现构建、Git、原子提交、基本验证 |
| 协作开发 | 多人并行、长期维护、代码评审 | Topic Branch、Issue、PR、CI、主分支保护 |
| 正式交付 | 对外发布、客户交付、可复用组件 | 版本策略、可追溯制品、发布说明、安全与依赖治理 |

这三种场景描述能力增长路径。个人项目也能通过 PR 进行自我评审，团队项目也可以从最小流程起步；是否引入一项
机制，取决于它控制的风险是否真实存在，以及项目能否持续维护该机制。

### 1.3 贯穿示例

本文使用 `sensor-controller` 作为通用示例工程。它包含一个 MCU 固件、少量主机测试和烧录工具，不绑定具体芯片：

```text
sensor-controller/
├── firmware/
├── tests/
├── tools/
├── docs/
├── hardware/
├── third_party/
├── CMakeLists.txt
├── README.md
└── .gitignore
```

示例中的仓库地址、用户名、板卡名和构建命令都需要替换成项目实际值。

### 1.4 术语

| 术语 | 本文含义 |
| --- | --- |
| 仓库（Repository） | Git 管理的项目历史及其工作目录；GitHub 仓库是其远程托管形式 |
| 权威输入 | 能够生成、构建或解释结果，且需要版本化保存的源文件与配置 |
| 生成文件 | 由工具从权威输入确定性地产生的文件 |
| 构建产物 | 编译、链接、打包或测试产生的结果，如 `.elf`、`.hex`、`.bin` 和报告 |
| 工作树（Working Tree） | 当前在文件系统中看到和编辑的文件 |
| 暂存区（Index） | 下一次 Commit 将记录的精确内容 |
| Commit | 一次带作者、时间、说明和父节点的仓库快照 |
| Branch | 指向某个 Commit 的可移动名称 |
| Remote | 本地仓库记录的远程仓库名称与地址 |
| Pull Request（PR） | 在 GitHub 上提出将一个分支的修改合入另一个分支的请求与讨论空间 |
| CI | 持续集成；由机器对变更执行可重复的检查、测试和构建 |
| Release | 面向使用者发布的版本说明和交付制品集合 |

---

# 第一部分：工程仓库组织

## 2. 目录设计的判断方法

### 2.1 目录服务于边界

嵌入式目录会受到代码生成器、IDE、SDK、构建系统、芯片数量和固件数量约束。目录设计首先回答以下问题：

- 哪些文件由开发者维护，哪些文件由工具生成？
- 哪些文件参与构建，哪些文件是构建结果？
- 哪些内容属于项目，哪些内容来自厂商或第三方？
- 哪个固件、板卡或主机工具拥有该文件？
- 新环境根据仓库内容能否完成构建和最小验证？
- 一个目录移动或替换后，影响范围能否被快速识别？

目录名只是一种表达。厂商工具固定了目录时，应保留能够稳定再生成和构建的物理布局，并在 README 中说明它与
项目逻辑架构的映射。

### 2.2 七类文件

提交文件前先判断其类别：

| 类别 | 典型内容 | 版本控制处理 |
| --- | --- | --- |
| 手写源码 | `.c`、`.h`、脚本、测试、构建定义 | 提交 |
| 配置源 | `.ioc`、SysConfig、Kconfig、链接脚本、板卡配置 | 提交 |
| 必需生成源码 | 启动文件、厂商生成初始化代码 | 无法在标准构建中生成时提交，并记录生成方式 |
| 可再生文件 | 自动生成文档、中间代码、依赖锁定结果 | 根据复现成本、审查价值和工具惯例决定 |
| 第三方内容 | SDK、库、设备驱动、许可证 | 固定来源与版本，记录本地修改 |
| 构建与测试结果 | `build/`、覆盖率、日志、临时固件 | 通常忽略；正式结果由 CI 或 Release 保存 |
| 本地或敏感数据 | 密钥、令牌、个人路径、设备序列号 | 不提交；提供无敏感值的示例配置 |

判断一个生成文件是否提交，可以依次检查：

1. 新环境能否通过仓库中已固定版本的工具生成？
2. 生成是否确定，是否会产生大量无意义差异？
3. 代码评审是否需要看到生成结果？
4. 工具是否要求生成文件存在后才能打开或构建工程？
5. 生成器停止供应时，仓库是否仍需具备维护能力？

结论写进 README 或 `docs/build.md`，避免不同开发者反复作出相反选择。

### 2.3 单一事实来源

同一项信息应存在明确的权威位置。例如：

- 引脚分配以生成器配置或板卡配置文件为准，文档引用该位置。
- 产品版本由一个构建系统可读取的版本源定义，发布流程从该位置取值。
- 工具链版本由构建说明、容器或工具链文件固定，不依赖开发者记忆。
- 第三方版本由依赖清单、Submodule Commit 或 Vendor 清单记录。

多个副本无法自动同步时，需要说明哪个副本是输入、哪个副本是派生结果，以及如何检查二者一致。

## 3. 推荐的仓库骨架

### 3.1 单固件工程

以下目录树适合不受厂商目录强约束、同时包含测试和工具的工程：

```text
sensor-controller/
├── firmware/                  # MCU 固件工程
│   ├── app/                   # 物理映射由架构文档解释
│   ├── domain/
│   ├── services/
│   ├── libraries/
│   ├── bsp/
│   ├── drivers/
│   ├── platform/
│   ├── config/
│   ├── startup/
│   └── linker/
├── tests/
│   ├── unit/                  # 可在主机执行的逻辑测试
│   ├── integration/           # 多模块、协议或端口组合测试
│   ├── hil/                   # 硬件在环脚本与测试描述
│   └── fixtures/              # 测试数据，不保存敏感现场数据
├── tools/                     # 烧录、生成、分析和维护工具
├── scripts/                   # 简短的仓库自动化入口
├── docs/                      # 架构、构建、协议、调试和维护文档
├── hardware/                  # 原理图、PCB、引脚和硬件版本资料
├── third_party/               # 可再分发的外部项目源码
├── assets/                    # 测试图片、界面资源等静态资产
├── models/                    # 版本化模型或模型获取清单
├── cmake/                     # 构建系统模块；按实际工具创建
├── .github/                   # GitHub 模板与 Actions
├── CMakeLists.txt             # 非交互构建入口示例
├── README.md
├── CHANGELOG.md
├── LICENSE
├── .gitignore
├── .gitattributes
└── .clang-format
```

工程不需要为了补齐目录树而创建空目录。`Inc/` 与 `Src/` 也不需要在每一级重复出现；模块可以将 `.c` 和 `.h`
放在同一目录，或者只在确实需要区分公共接口和实现时设置 `include/`、`src/`。

### 3.2 厂商生成工程

STM32CubeMX、TI SysConfig 和其他生成器通常拥有一部分物理结构。可采用“保留生成区、建立手写区”的布局：

```text
sensor-controller/
├── Core/                      # 生成器拥有或共同维护
├── Drivers/                   # 厂商 HAL/CMSIS
├── Middlewares/               # 生成器引入的中间件
├── User/                      # 项目手写固件
│   ├── App/
│   ├── Domain/
│   ├── Services/
│   ├── BSP/
│   ├── Drivers/
│   └── Libraries/
├── tests/
├── tools/
├── docs/
├── project.ioc                # 生成配置源示例
└── README.md
```

项目应记录：

- 生成器和 SDK 的准确版本；
- 重新生成的入口与步骤；
- 哪些目录允许手写，哪些区域可能被覆盖；
- 生成后必须执行的格式化、补丁或一致性检查；
- 生成文件是否提交及其原因。

代码生成器提供的用户代码保护区只解决覆盖问题，不自动形成良好的模块边界。手写功能仍应根据
`ARCHITECTURE_STANDARD.md` 组织和依赖。

### 3.3 多固件和混合仓库

同一产品包含 Bootloader、主控制器、协处理器和主机工具时，可以按可独立构建的交付单元组织：

```text
product/
├── firmware/
│   ├── bootloader/
│   ├── main_controller/
│   └── sensor_node/
├── host/
│   ├── configuration_tool/
│   └── protocol_library/
├── tools/
├── models/
├── protocols/
├── tests/
├── docs/
└── hardware/
```

适合放在同一仓库的内容通常具有共同版本、共同协议、原子修改需求或统一发布节奏。构建权限、团队、发布周期和
历史完全独立时，拆分仓库能降低耦合。无论选择哪种方式，都应给每个交付单元提供明确构建入口，并让根目录能够
执行或导航到全部关键检查。

## 4. 重点目录和文件

### 4.1 `third_party/`、`vendor/` 与 SDK

推荐使用以下含义：

- `third_party/`：第三方开源库、设备驱动或可再分发组件。
- `vendor/`：直接纳入仓库、由项目固定和可能打补丁的外部源码快照。
- 厂商 SDK 目录：遵循 SDK 原名或生成器布局，例如 `Drivers/CMSIS`。

每个外部组件至少记录名称、上游地址、版本或 Commit、许可证、获取方式、本地修改和更新方法。常见引入方式：

| 方式 | 优点 | 主要成本 | 适用情况 |
| --- | --- | --- | --- |
| 包管理器与锁文件 | 更新和依赖解析方便 | 依赖生态和网络可用性 | 主机工具、支持良好的嵌入式包生态 |
| Git Submodule | 上游历史和版本边界清晰 | 克隆、切换和 CI 操作更复杂 | 独立维护、需要保留仓库身份的依赖 |
| Vendor 快照 | 离线构建简单，补丁易审查 | 更新需要人工同步 | 小型稳定库、比赛或封闭构建环境 |
| 构建时下载 | 仓库较小 | 来源失效和供应链风险 | 已固定版本、校验和与缓存策略的环境 |

直接修改第三方目录会增加升级成本。确需修改时，应保存清晰补丁、提交说明或 `PATCHES.md`，并保留原始版本标识。

### 4.2 `tools/` 与 `scripts/`

`tools/` 放置具有独立逻辑、依赖或测试的工具；`scripts/` 放置构建、烧录、检查等短入口。工具应满足：

- 参数和退出码稳定，失败时返回非零状态；
- 不依赖个人绝对路径；
- 能在非交互环境运行；
- 依赖版本可以安装或复现；
- 默认不覆盖不可恢复的数据；
- Python 工具遵循 PEP 8，并由项目选择的格式化和检查工具约束。

脚本命名表达动作，例如 `build.sh`、`flash.sh`、`check.sh`。复杂逻辑增长后，将实现迁入可测试的工具模块，脚本只保留入口。

### 4.3 `docs/` 与 `hardware/`

`docs/` 保存需要随代码评审和版本同步的工程知识，如：

- 架构和模块设计；
- 构建、烧录与调试；
- 通信协议和存储格式；
- 测试方法、硬件在环连接和故障排查；
- 架构决策记录（ADR）与版本迁移说明。

`hardware/` 保存原理图、PCB、BOM、引脚表、机械资料和硬件版本差异。大型 CAD 源文件要考虑 Git LFS、独立制品库和
授权边界；导出的 PDF 不能取代可维护源文件，但可以提高审查和查阅便利性。

### 4.4 `assets/` 与 `models/`

小型、稳定、构建或测试必需的资源可以提交。大型数据集和模型应记录：

- 来源、许可证和使用限制；
- 内容哈希或版本；
- 获取与转换命令；
- 输入输出规格、量化方式和目标平台；
- 是否使用 Git LFS 或外部制品存储。

模型训练数据、生成模型和固件内置模型属于不同制品，应分别追踪来源。敏感数据和来源不明的数据不能进入仓库。

### 4.5 `build/`、`dist/` 与测试输出

- `build/`：中间文件和本地构建树，通常加入 `.gitignore`。
- `dist/`：本地打包结果，通常不提交，由 Release 或 CI Artifact 保存。
- 测试日志、覆盖率和静态分析报告：本地生成，CI 按保留期上传。

正式固件需要从确定的 Commit 和工具链重新构建。将日常二进制提交到源码历史会放大仓库并削弱来源追踪；必须随源码
保存的黄金镜像或协议样本应放入明确的测试数据目录，并记录用途和更新条件。

## 5. 根目录的工程契约

### 5.1 `README.md`

README 是项目入口，至少回答：

1. 项目解决什么问题，目前处于什么状态？
2. 支持哪些 MCU、板卡和硬件版本？
3. 需要什么工具链、SDK、生成器和调试器？
4. 如何配置、构建、烧录并观察到成功结果？
5. 仓库关键目录分别负责什么？
6. 如何执行基础测试，已知限制有哪些？
7. 从哪里获取版本、协议、架构和贡献说明？

README 的 Quick Start 应由一台未配置过该项目的环境验证。模板见
[`templates/project/README.template.md`](templates/project/README.template.md)。

### 5.2 `.gitignore`

`.gitignore` 只影响尚未跟踪的文件。已经提交的文件需要先从索引移除，忽略规则才会生效：

```sh
git rm --cached path/to/generated-file
```

忽略项应来自真实工具输出，包括构建目录、IDE 本地状态、日志、缓存、虚拟环境和本地秘密。不能用过宽规则隐藏可能
属于源码的文件；例如全局忽略全部 `.bin` 会同时隐藏必要测试样本。

### 5.3 `.gitattributes`

`.gitattributes` 用于统一文本识别、行尾和二进制差异行为。示例：

```gitattributes
* text=auto
*.c text eol=lf
*.h text eol=lf
*.md text eol=lf
*.sh text eol=lf
*.bat text eol=crlf
*.png binary
*.pdf binary
*.elf binary
*.bin binary
```

引入该文件后，可在独立 Commit 中执行一次受控的规范化，并确认差异只涉及预期行尾：

```sh
git add --renormalize .
git diff --cached --check
```

### 5.4 构建入口与工具链

专业仓库应提供非交互构建入口，例如 CMake Presets、Make、West、PlatformIO 或项目脚本。入口需要明确：

- 目标板和构建配置；
- 工具链及版本；
- 生成步骤；
- 输出路径；
- 成功和失败退出码；
- 清理、测试和烧录方式。

IDE 工程可以作为入口之一，但关键构建不能只存在于个人工作区状态。CI 与本地尽量调用同一个底层命令。

### 5.5 许可证、变更和协作文件

| 文件 | 使用时机 | 主要内容 |
| --- | --- | --- |
| `LICENSE` | 对外分发或开源 | 项目授权条款 |
| `THIRD_PARTY_NOTICES.md` | 分发第三方组件 | 组件、版本、许可证和声明 |
| `CHANGELOG.md` | 有稳定版本和用户 | 用户可感知的新增、变更、修复和兼容影响 |
| `CONTRIBUTING.md` | 多人或公开贡献 | 环境、分支、检查、PR 和评审方式 |
| `SECURITY.md` | 对外交付或公开仓库 | 支持版本和私密漏洞报告渠道 |
| `.env.example` | 工具需要环境变量 | 变量名与无敏感值示例 |

仓库内不能保存访问令牌、私钥、Wi-Fi 密码、真实生产地址和其他凭据。秘密一旦进入 Commit，即使之后删除文件也仍在历史中；
应立即轮换凭据，再评估历史清理和通知范围。

### 5.6 工具配置与 `.github/`

工具配置应放在工具能够自动发现、开发者和 CI 共同使用的位置：

| 文件或目录 | 典型职责 |
| --- | --- |
| `.editorconfig` | 编辑器可执行的编码、缩进、行尾和文件末尾换行基础设置 |
| `.clang-format`、`.clang-tidy` | C/C++ 格式化和静态检查配置 |
| `CMakePresets.json` | 可复现的配置、构建和测试预设 |
| `pyproject.toml` | Python 工具的构建、依赖和质量工具配置 |
| `Doxyfile` | API 文档输入、排除目录、警告和输出设置 |
| `.github/ISSUE_TEMPLATE/` | Issue Forms 和入口配置 |
| `.github/workflows/` | CI、HIL、Release 和维护自动化 |
| `.github/PULL_REQUEST_TEMPLATE.md` | PR 默认说明结构 |
| `.github/CODEOWNERS` | 团队关键路径的自动评审请求 |

配置文件是工程行为的一部分，需要与工具版本一同评审。只生成在个人主目录或 IDE 工作区中的关键设置无法支持可复现构建；
与个人偏好相关且不影响项目结果的设置也无需全部提交。

---

# 第二部分：Git 从基础到日常开发

## 6. Git 的数据模型

### 6.1 Git 记录快照

Git 将 Commit 组织成有向历史。每个 Commit 包含项目快照、作者信息、说明和父 Commit。Branch 只是指向历史节点的名称，
切换分支会让工作树呈现该分支所指向的内容。

日常修改经过三个主要区域：

```text
工作树 -- git add --> 暂存区 -- git commit --> 本地仓库 -- git push --> 远程仓库
   ^                     |
   |---- git restore ----|
```

`git add` 的含义是把当前文件内容放入下一次 Commit，而不只是“让 Git 知道这个文件”。同一文件可以一部分已暂存、
另一部分仍在工作树，因此提交前需要分别查看两类差异。

### 6.2 HEAD、Branch 与 Remote-tracking Branch

- `HEAD` 表示当前检出的 Commit，通常间接指向当前分支。
- `main` 是本地分支。
- `origin` 是远程仓库的惯用名称，本身不是分支。
- `origin/main` 是最近一次 `fetch` 后，本地记录的远程 `main` 状态。

`git fetch` 更新远程跟踪信息，不修改当前工作树。`git pull` 通常等价于先 `fetch`，再按配置执行 merge 或 rebase。
学习阶段先分开执行，可以更清楚地观察历史和处理冲突。

## 7. 初始配置与练习仓库

### 7.1 身份和默认设置

首次使用 Git 时配置提交身份：

```sh
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
git config --global init.defaultBranch main
```

查看来源和最终值：

```sh
git config --list --show-origin
```

提交邮箱会进入历史。公开仓库可以使用 GitHub 提供的 noreply 地址；团队项目遵循组织身份政策。

推荐让 Git 在提交时拒绝混合行尾问题，并由 `.gitattributes` 决定仓库行尾：

```sh
git config --global core.safecrlf warn
```

`core.autocrlf` 的选择与操作系统和团队策略有关。跨 Windows、WSL 和 Linux 的工程优先由 `.gitattributes` 明确定义，
并在团队内统一 Git 配置。

### 7.2 建立安全练习仓库

```sh
mkdir git-practice
cd git-practice
git init
printf '# Git practice\n' > README.md
git add README.md
git commit -m "docs: add initial README"
git status
git log --oneline --decorate --graph --all
```

该目录只用于练习。涉及 `reset --hard`、`clean`、rebase 和恢复的命令先在这里验证。

## 8. 日常修改闭环

### 8.1 开始工作前

```sh
git status --short --branch
git fetch --prune origin
git log --oneline --decorate --graph --all -n 20
```

确认当前分支、未提交修改和远程状态。多人项目从最新目标分支创建 Topic Branch：

```sh
git switch main
git merge --ff-only origin/main
git switch -c feat/sensor-timeout
```

`--ff-only` 在本地分支已经分叉时拒绝产生意外 Merge Commit，让开发者先判断历史关系。

### 8.2 查看与暂存

```sh
git status
git diff
git diff --check
git add path/to/file.c path/to/file.h
git diff --cached
```

- `git diff`：工作树相对暂存区的变化。
- `git diff --cached`：下一次 Commit 的内容。
- `git diff --check`：尾随空白和部分空白错误。

一个文件混入多个目的时，可交互式选择补丁：

```sh
git add -p path/to/file.c
```

取消暂存但保留文件修改：

```sh
git restore --staged path/to/file.c
```

丢弃尚未暂存的修改会造成数据丢失，先检查差异：

```sh
git diff -- path/to/file.c
git restore path/to/file.c
```

### 8.3 验证和提交

提交前运行与风险相称的检查：格式、编译、静态分析、主机测试、目标板验证或资源预算。随后再次确认暂存区：

```sh
git diff --cached --check
git diff --cached
git commit
```

提交后检查：

```sh
git show --stat --oneline HEAD
git status
```

一个健康闭环的结果是：Commit 只表达一个主要目的，说明能够解释原因，验证证据可追溯，工作树中剩余修改也符合预期。

## 9. Commit 设计与 Conventional Commits

### 9.1 原子 Commit

原子 Commit 具备以下特征：

- 只有一个可以清楚命名的主要目的；
- 单独检出后仍能构建，或明确说明无法独立验证的原因；
- 实现、直接测试和必要文档保持一致；
- 不混入无关格式化、重命名或临时调试；
- 可以独立评审、回退或挑选。

修改规模由语义决定。一个完整接口调整可能涉及多个文件，仍然是一个原子 Commit；两个互不相关的单行修复应拆开。

### 9.2 提交消息

本文示例采用 [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/)：

```text
<type>(<scope>)!: <subject>

<body>

<footer>
```

常用 Type：

| Type | 用途 | 示例 |
| --- | --- | --- |
| `feat` | 新增用户或系统能力 | `feat(protocol): add heartbeat frame` |
| `fix` | 修复错误行为 | `fix(bsp): correct encoder timer mapping` |
| `refactor` | 不改变外部行为的内部整理 | `refactor(parser): isolate checksum state` |
| `perf` | 性能优化 | `perf(filter): reduce update latency` |
| `test` | 测试代码或测试设施 | `test(pid): cover output saturation` |
| `docs` | 仅文档 | `docs(readme): add flashing steps` |
| `build` | 构建系统或依赖 | `build(cmake): add release preset` |
| `ci` | 持续集成 | `ci: build supported targets` |
| `chore` | 无更合适类型的维护 | `chore: update license year` |
| `revert` | 回退已有 Commit | `revert: disable experimental transport` |

标题使用祈使语气的英文短句，不加句号，描述结果。Scope 表达稳定区域，如 `bsp`、`protocol`、`scheduler`；无需为了形式
强行添加。正文说明动机、方案边界和无法从差异直接看出的权衡：

```text
fix(storage): reject truncated parameter records

Validate the payload length before reading the CRC field. This prevents a
partially written record from being accepted after power loss.

Closes: #42
```

破坏兼容的修改使用 `!` 或 `BREAKING CHANGE:` Footer，并说明迁移方式。提交消息正文可以使用中文或英文，仓库内保持一致即可。

### 9.3 修改最近一次提交

最近 Commit 尚未共享时，可以补入遗漏并修改说明：

```sh
git add path/to/missed-file.c
git commit --amend
```

Amend 会创建新的 Commit ID。已经推送且被他人使用的 Commit 不应随意改写；新增修复 Commit 或通过后续整理由团队处理。

## 10. 分支与集成方式

### 10.1 Topic Branch

Topic Branch 将一项功能、修复或实验与稳定主线隔离。常用命名：

```text
feat/sensor-timeout
fix/uart-overrun
refactor/parameter-store
test/hil-power-cycle
spike/can-fd-driver
```

名称使用小写和连字符，表达目标，不加入人员姓名。分支应有明确入口和出口：合并、形成结论后关闭，或确认废弃后删除。

个人项目中的低风险小修改可以在干净且健康的 `main` 上直接形成原子 Commit。涉及实验、跨文件功能、历史整理或希望通过
PR 自查时，Topic Branch 能提供更安全的边界。协作项目通常通过规则保护 `main`，所有修改从分支进入。

### 10.2 Merge

Merge 组合两条历史，并在需要时创建 Merge Commit：

```sh
git switch main
git merge --no-ff feat/sensor-timeout
```

它保留分支上每个 Commit 和分叉关系，适合 Commit 本身经过整理、分支身份有审计价值的工作。历史会出现分叉和合并节点。

### 10.3 Rebase

Rebase 把当前分支的 Commit 重新应用到新的基点：

```sh
git switch feat/sensor-timeout
git fetch origin
git rebase origin/main
```

出现冲突时：

```sh
git status
# 编辑并解决冲突
git add path/to/resolved-file.c
git rebase --continue
```

放弃本次 rebase：

```sh
git rebase --abort
```

Rebase 会产生新的 Commit ID，适合整理尚未共享或明确由本人维护的 Topic Branch。对他人已经基于其开发的公共分支执行
rebase，会迫使其他人重新协调历史。

### 10.4 Squash

Squash 将多个开发过程 Commit 整理成更少的逻辑 Commit。可以在交互式 rebase 中完成，也可以由 GitHub 的
Squash and merge 完成。它适合保留 PR 的讨论和验证记录，同时让主分支每个 PR 对应一个清晰 Commit。

Squash 会丢失分支内部 Commit 的独立身份；对经过精心组织、需要逐步追踪或单独回退的 Commit，保留原 Commit 更有价值。

### 10.5 选择依据

| 目标 | 常见选择 | 需要接受的特征 |
| --- | --- | --- |
| 主线简洁，每个 PR 一个 Commit | Squash merge | 分支内部 Commit 不进入主线 |
| 主线线性，同时保留各 Commit | Rebase merge | 分支 Commit 被重写；需保证每个 Commit 质量 |
| 保留完整分支拓扑和原 Commit | Merge commit | 历史包含合并节点 |

仓库应在贡献说明中记录允许的方式。选择应围绕调试、回退、审计和维护习惯，不能只依据提交图的外观。

### 10.6 分支模型的选择

常见模型围绕发布节奏和并行维护需求形成：

| 模型 | 主要结构 | 适用情况 | 维护重点 |
| --- | --- | --- | --- |
| 主分支加短期 Topic Branch | `main` 始终可集成，功能通过短分支进入 | 个人、比赛、持续集成和大多数团队 | 分支短小、频繁同步、CI 可靠 |
| Trunk-based Development | 很短的分支或受控直接提交，未完成功能由开关隔离 | 自动化成熟、集成频率高的团队 | 主线质量、功能开关生命周期、快速修复 |
| Release Branch | `main` 推进下一版本，`release/x.y` 稳定候选版本 | 验证窗口较长或同时维护多个版本 | 修复回合并、版本归属、分支退役 |
| Git Flow | 长期 `develop` 加功能、Release 和 Hotfix 分支 | 发布批次固定、流程角色明确的产品 | 分支同步和合并成本较高 |

`release/x.y` 只在候选版本冻结后主线仍需继续开发，或一个已发布系列需要维护时产生价值。`hotfix/...` 表达对已发布版本的
紧急修复来源，修复仍需回到所有受影响的活跃分支。单一主线即可完成发布的项目不需要预先创建长期 Release 或 Develop 分支。

## 11. Remote、同步和冲突

### 11.1 连接远程仓库

```sh
git remote add origin https://github.com/OWNER/sensor-controller.git
git remote -v
git push -u origin main
```

`-u` 建立上游关系，后续可以直接使用 `git push` 和 `git pull`。认证应使用系统凭据管理器、SSH Key 或 GitHub 支持的
令牌方式，不能把凭据写入仓库远程地址、脚本或配置文件。

### 11.2 Fetch 与安全同步

```sh
git fetch --prune origin
git branch --all --verbose --verbose
git log --oneline --left-right main...origin/main
```

`--prune` 删除已经不存在的远程跟踪引用，不会删除本地工作分支。同步 `main`：

```sh
git switch main
git merge --ff-only origin/main
```

同步 Topic Branch 可以选择 merge 或 rebase，遵循仓库策略：

```sh
git switch feat/sensor-timeout
git merge origin/main
```

或：

```sh
git switch feat/sensor-timeout
git rebase origin/main
git push --force-with-lease
```

Rebase 后远程 Topic Branch 与本地历史不同，需要更新远程引用。`--force-with-lease` 会在远程分支出现未预期更新时拒绝覆盖，
安全性高于 `--force`；执行前仍应确认该分支的所有权和团队约定。

### 11.3 冲突处理

冲突表示 Git 无法自动判断目标内容，不代表任何一方一定错误。处理步骤：

1. 阅读 `git status`，确认正在进行 merge 还是 rebase。
2. 理解共同基线、当前分支和另一分支的意图。
3. 编辑冲突文件，删除标记并形成正确的最终代码。
4. 执行格式、构建和相关测试。
5. `git add` 已解决文件。
6. 继续操作或在不确定时中止。

```sh
git merge --abort
git rebase --abort
```

不能只为了消除标记机械选择 ours/theirs。生成文件发生冲突时，优先合并权威配置源，再用固定版本工具重新生成。

## 12. 撤销、恢复与危险操作

### 12.1 按数据所在区域选择命令

| 目标 | 命令 | 结果 |
| --- | --- | --- |
| 取消文件暂存 | `git restore --staged <path>` | 修改保留在工作树 |
| 丢弃未暂存修改 | `git restore <path>` | 工作树修改被删除 |
| 回退共享 Commit | `git revert <commit>` | 新增一个反向 Commit，保留历史 |
| 查看引用移动记录 | `git reflog` | 帮助定位丢失的 Commit |
| 临时保存修改 | `git stash push -u` | 修改离开工作树，进入 Stash |
| 复制指定 Commit | `git cherry-pick <commit>` | 在当前分支创建内容等价的新 Commit |
| 删除未跟踪文件 | `git clean` | 可能永久删除，先使用 `-n` 预览 |
| 移动当前分支 | `git reset` | 可能同时改变暂存区和工作树 |

### 12.2 共享历史使用 Revert

已经推送、合并或发布的错误 Commit，通常用 Revert：

```sh
git switch -c revert/broken-change origin/main
git revert <commit-sha>
# 构建和验证
git push -u origin revert/broken-change
```

这会留下明确审计记录，也不会让协作者已有历史失效。回退 Merge Commit 需要理解主线父节点，不能在不确认 `-m` 含义时执行。

### 12.3 使用 Reflog 恢复

误删分支或错误 reset 后，先停止继续改写历史：

```sh
git status
git reflog --date=local
git branch rescue/<name> <recovered-sha>
git show <recovered-sha>
```

先建立救援分支，再评估如何恢复。Reflog 是本地记录，有过期策略，也不会自动存在于另一台电脑。

### 12.4 Reset 与 Clean

`git reset --soft`、`--mixed`、`--hard` 分别以不同方式影响 Branch、暂存区和工作树。`--hard` 会丢弃已跟踪文件修改；
`git clean -fd` 会删除未跟踪文件和目录。执行前至少完成：

```sh
git status --short --branch
git diff
git diff --cached
git clean -nd
```

存在任何不确定性时，建立临时分支、复制未跟踪数据或使用 Stash。Stash 适合短期上下文切换，不适合作为长期备份。

## 13. 版本、Tag 与 Changelog

### 13.1 版本对应兼容面

固件版本需要说明它对谁承诺兼容。兼容面可能包括：

- 上位机或通信协议；
- Bootloader 与升级格式；
- 参数存储和校准数据；
- 板卡与硬件修订版；
- 公开 C API 或可复用驱动接口；
- 工具链、配置文件和生产测试流程。

在定义兼容面后，[Semantic Versioning 2.0.0](https://semver.org/) 才能准确应用：

- MAJOR：已声明兼容面发生破坏性变化；
- MINOR：向后兼容地增加能力；
- PATCH：向后兼容地修复错误。

原型阶段可以使用 `0.y.z`；发布候选使用 `1.2.0-rc.1`。产品内部版本方案不同于 SemVer 时，应明确各段含义和递增条件。

### 13.2 Tag

Tag 将版本名绑定到具体 Commit。正式版本优先使用带说明的 Tag：

```sh
git tag -a v1.2.0 -m "Release v1.2.0"
git show v1.2.0
git push origin v1.2.0
```

已公开 Tag 应视为不可移动标识。发布内容错误时创建修复版本或新的候选版本，让使用者能够稳定定位历史来源。

### 13.3 Changelog

Changelog 面向使用者，记录有价值的 Added、Changed、Deprecated、Removed、Fixed 和 Security 内容。它不需要罗列全部 Commit，
需要突出兼容性、硬件范围、升级步骤和已知问题。可采用 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) 的组织方式。

### 13.4 Commit 与 Tag 签名

Git 可以使用 SSH、GPG 或平台支持的方式对 Commit 和 Tag 签名。签名证明对象与受验证密钥之间的关系，不能证明代码正确、
评审充分或提交者设备安全。

签名适合对身份来源、审计或正式发布有明确要求的项目。启用前需要确定密钥生成、备份、轮换、吊销、机器人身份和开发者
入职流程。Ruleset 要求签名时，自动提交、网页合并和 Squash 的行为也要在测试仓库验证。

带签名 Tag 的典型命令：

```sh
git tag -s v1.2.0 -m "Release v1.2.0"
git tag -v v1.2.0
```

签名私钥不能存入仓库或普通 CI 变量；正式制品签名还需要独立考虑密钥保护、授权和可验证发布流程。

---

# 第三部分：GitHub 从第一次推送到 Pull Request

## 14. GitHub 与本地 Git

Git 能在本地完整工作。GitHub 提供远程托管、权限、Issue、PR、Review、Actions、Release 和安全治理。网页上的修改最终也会
形成 Git Commit；PR、Review 和 Actions 记录则属于 GitHub 平台数据，不会随普通 `git clone` 完整复制。

### 14.1 创建远程仓库

已有本地仓库时，在 GitHub 网页选择 **New repository**：

1. 输入仓库名称和可见性。
2. 本地已有 README、License 和 `.gitignore` 时，不在网页重复初始化。
3. 创建后复制 HTTPS 或 SSH 地址。
4. 在本地添加 `origin` 并首次推送。

```sh
git remote add origin https://github.com/OWNER/sensor-controller.git
git push -u origin main
```

网页和具体入口会随 GitHub 更新；以仓库首页与官方文档的当前界面为准。

### 14.2 Clone、Fork 与 Branch

- Clone：把远程仓库及其历史复制到本地。
- Fork：在自己的 GitHub 账户下创建关联副本，常用于无上游写权限的开源贡献。
- Branch：同一仓库内的开发线，团队成员有写权限时通常直接推送 Topic Branch。

```sh
git clone https://github.com/OWNER/sensor-controller.git
cd sensor-controller
git switch -c docs/first-contribution
```

Fork 工作流还会将原仓库记录为 `upstream`：

```sh
git remote add upstream https://github.com/UPSTREAM/sensor-controller.git
git fetch upstream
```

## 15. Issue：定义可完成的工作

Issue 用于记录 Bug、功能、任务和可追踪讨论。它不是提交代码的前置仪式；价值在于把问题、约束和完成条件留在变更之外，
使实现和评审拥有共同目标。

### 15.1 Bug Issue

嵌入式 Bug 通常需要：

- 固件版本或 Commit SHA；
- MCU、板卡及硬件修订版；
- 编译器、SDK、构建配置和相关外设；
- 可重复步骤、复现率和最后正常版本；
- 期望与实际行为；
- 错误码、日志、波形、总线捕获或最小复现；
- 电源、时钟、温度和时序等相关条件；
- 已移除凭据和私人数据的证据。

### 15.2 Feature Issue

功能请求先描述问题和可观察目标，再讨论实现：

- 使用者或系统需要什么能力？
- 验收条件是什么？
- 硬件、实时性、内存、功耗和兼容约束是什么？
- 哪些内容明确不在本次范围？
- 是否影响协议、存储、Bootloader 或生产流程？

模板见 [`templates/github/ISSUE_TEMPLATE/`](templates/github/ISSUE_TEMPLATE/)。小型个人修改可以直接从清楚的 Commit 开始；
跨日工作、Bug 调查和需要保留决策的修改更适合先建 Issue。

### 15.3 Labels、Milestones 和 Projects

- Labels 用于稳定分类，如 `type:bug`、`area:bsp`、`priority:high`、`status:blocked`。
- Milestone 表示一个版本或阶段的目标集合。
- Projects 适合跨多个 Issue 和 PR 的计划与状态追踪。
- Discussions 适合开放问答、想法和尚未形成可执行任务的讨论。

分类体系从少量真实需求起步。大量重叠标签会增加维护成本，也不能代替 Issue 本身的清晰描述。

## 16. 第一次 Pull Request

GitHub 将 PR 定义为把一个分支的修改提议合入另一个分支。PR 页面集中保存差异、讨论、Review、检查结果和合并记录。
以下步骤可以在个人测试仓库中完整练习。

### 16.1 创建修改分支

```sh
git switch main
git fetch origin
git merge --ff-only origin/main
git switch -c docs/add-build-note
```

修改 README 后检查并提交：

```sh
git status
git diff
git add README.md
git diff --cached
git commit -m "docs(readme): add build troubleshooting note"
git push -u origin docs/add-build-note
```

### 16.2 在网页创建 PR

推送新分支后，GitHub 仓库首页通常显示 **Compare & pull request**。也可以进入 **Pull requests** → **New pull request**：

1. Base 选择目标分支 `main`。
2. Compare 选择 `docs/add-build-note`。
3. 确认提交列表和 Files changed 只包含预期修改。
4. 标题概括结果，可继续采用 Conventional Commits 格式。
5. 正文写明目标、修改、验证、风险和关联 Issue。
6. 尚未准备合并时选择 Draft PR。
7. 创建后观察 Checks、Files changed 和 Conversation。

PR 描述模板见 [`templates/github/PULL_REQUEST_TEMPLATE.md`](templates/github/PULL_REQUEST_TEMPLATE.md)。创建 PR 不会立即合并代码，
也不会停止继续推送；同一源分支的新 Commit 会自动进入该 PR。

### 16.3 使用 `gh` CLI 创建

熟悉网页流程后，可以使用 GitHub CLI：

```sh
gh auth login
gh pr create --base main --fill
gh pr view --web
gh pr checks --watch
```

CLI 适合提高效率和自动化，网页仍然适合查看完整讨论、逐行差异和仓库设置。

### 16.4 更新 PR

根据评审修改代码后，正常提交并推送：

```sh
git add path/to/changed-file
git commit -m "fix: address review finding"
git push
```

是否保留 review-fix Commit，取决于最终合并方式。Squash merge 会在合并时整理；保留 Commit 的工作流可以在合并前交互式
rebase，但只应改写由自己控制的 Topic Branch。

## 17. Review、Checks 与合并

### 17.1 Review 的对象

有效 Review 检查变更是否解决目标，并评估：

- 接口、所有权和依赖是否清楚；
- 错误路径、超时、边界和恢复是否完整；
- ISR、DMA、并发和生命周期是否安全；
- Flash、RAM、栈、实时性、功耗和协议是否受影响；
- 生成配置、代码和文档是否一致；
- 测试是否覆盖风险，证据能否复现；
- 是否包含秘密、个人路径、无关文件或来源不明资产。

评论可以是问题、建议或必须解决的缺陷。提交者应通过代码、说明或有依据的讨论逐项处理；Conversation resolution 表示讨论
已经形成结论，不等价于机械点击完成。

### 17.2 Checks

Checks 通常来自 GitHub Actions 或外部 CI。它们对同一 Commit 执行格式、构建、测试和分析。红色检查先打开日志确认根因；
取消、跳过或基础设施故障应有解释。需要由 Ruleset 强制的检查名称要保持稳定。

CI 不能证明所有硬件行为。PR 同时记录目标板、硬件版本、构建配置、操作步骤和观察结果，能把人工验证纳入可追溯证据。

### 17.3 GitHub 三种合并方式

| 网页选项 | 主分支结果 | 适用特点 |
| --- | --- | --- |
| Create a merge commit | 保留原 Commit，并增加 Merge Commit | 保留完整分支拓扑和上下文 |
| Squash and merge | PR 合为一个新 Commit | 主线简洁，开发过程 Commit 无需永久保留 |
| Rebase and merge | 原 Commit 逐个重放到目标分支 | 线性历史，要求分支 Commit 本身质量较高 |

仓库设置可以只允许部分方式；Ruleset 的合并方式规则也必须与仓库设置一致。个人仓库可先练习 Squash merge，它降低整理临时
Commit 的负担；理解三种历史结果后，再根据项目的回退、审计和调试需求固定策略。

### 17.4 合并后的本地清理

网页合并并删除远程分支后：

```sh
git switch main
git fetch --prune origin
git merge --ff-only origin/main
git branch -d docs/add-build-note
```

`-d` 只删除 Git 判断已合并的分支。Squash merge 后，本地 Git 可能认为原分支未合并，因为主线得到的是新 Commit；确认 PR
确已合并且内容存在后，才使用 `git branch -D` 删除本地分支。

---

# 第四部分：自动化、保护与交付

## 18. GitHub Actions 与持续集成

### 18.1 从本地命令建立 CI

CI 应调用已经在本地验证的非交互命令。嵌入式工程常见顺序：

1. 文本卫生和格式检查；
2. 构建系统配置检查；
3. 主机单元测试与静态分析；
4. 支持目标和配置的交叉编译矩阵；
5. Flash、RAM、栈或镜像尺寸预算；
6. 需要设备时执行受控 HIL；
7. 上传日志、报告和临时固件 Artifact。

基础模板见 [`templates/github/workflows/ci.yml`](templates/github/workflows/ci.yml)。模板包含占位符，完成工具链固定和命令替换后
才能启用。

### 18.2 触发与并发

常见触发：

- `pull_request`：验证候选变更；
- `push` 到 `main`：验证合并后的真实主线；
- Tag Push：构建正式版本；
- `workflow_dispatch`：受控手动运行；
- `schedule`：周期性依赖、长测或静态分析。

同一 PR 连续推送时，旧运行通常可以取消。正式发布和操作硬件的工作流要使用并发组或资源锁，防止两个任务同时访问设备。

### 18.3 权限和不可信输入

Workflow 顶层先设置最小权限：

```yaml
permissions:
  contents: read
```

只在确有写入需求的 Job 提升权限。第三方 Action 固定到经过核对的完整 Commit SHA，并通过 Dependabot 或维护流程更新。
Tag 便于阅读，但可以被上游移动；完整 SHA 才提供不可变引用。

来自 PR 标题、分支名、Issue 或外部事件的数据属于不可信输入。不要把表达式直接拼入 Shell 脚本；通过环境变量传递并正确引用。
来自 Fork 的代码不能在持有写权限、生产秘密或内网访问能力的 Runner 上直接执行。

### 18.4 Cache、Artifact 和 Release

- Cache 加快依赖与构建过程，允许失效，不是结果的唯一副本。
- Workflow Artifact 保存一次运行产生的日志、报告和候选固件，具有保留期。
- Release Asset 面向版本使用者，是正式版本的一部分。

Artifact 名称应包含目标、配置和 Commit；正式制品还应包含版本、硬件目标、校验和、构建元数据及必要的符号或 map 文件。

### 18.5 HIL 与自托管 Runner

自托管 Runner 能访问调试器、串口、电源控制器和目标板，也扩大了安全边界。成熟的 HIL 环境需要：

- Runner 与设备唯一标识；
- 自动烧录、复位、超时、日志收集和结果判定；
- 设备并发锁、故障隔离和离线恢复；
- 区分产品失败、设备失败和基础设施失败；
- 最小账户权限、受控网络和补丁维护；
- 禁止不可信 Fork PR 直接访问设备网络和长期秘密。

手工硬件验证仍可作为起点，但应使用固定步骤和结果表记录，逐步自动化高频、确定和高价值的用例。

## 19. Rulesets、权限与仓库治理

### 19.1 主分支保护

GitHub Ruleset 可以限制删除和 Force Push，要求 PR、Review、状态检查、线性历史、签名或指定合并方式。配置前先确认实际
协作方式和 CI 稳定性；将尚未成功运行的 Job 直接设为必需检查，会让所有 PR 无法合并。

建议按风险逐步启用：

| 阶段 | 可考虑的设置 |
| --- | --- |
| 个人练习 | 禁止删除和 Force Push `main`；保留管理员恢复路径 |
| 常规协作 | 要求 PR、关键 Checks、讨论解决和至少一名适当 Reviewer |
| 正式交付 | 保护 Release Tag、限制旁路、要求发布检查和审计 |

多个 Ruleset 和 Branch Protection 同时适用时，限制会叠加。功能、套餐和私有仓库可用性可能不同，配置时核对 GitHub 当前说明。

### 19.2 权限最小化

- 仓库管理员只授予需要配置和恢复的人员。
- 日常开发使用 Write 或更小权限。
- 自动化使用专用 Token、GitHub App 或 `GITHUB_TOKEN`，并限制 Scope。
- 生产签名、部署和发布使用受保护 Environment 与审核机制。
- 离职、设备丢失和项目结束时及时撤销权限和密钥。

个人仓库也应启用多因素认证，使用操作系统凭据存储或硬件安全能力保护认证信息。

### 19.3 CODEOWNERS 与关键路径评审

团队可以用 `.github/CODEOWNERS` 将 Bootloader、板卡定义、协议、安全和发布工作流等路径映射给熟悉该范围的人员或团队。
匹配的代码所有者需要具有仓库写权限。示例：

```text
/firmware/bootloader/  @organization/boot-team
/hardware/             @organization/hardware-team
/.github/workflows/    @organization/release-team
```

CODEOWNERS 用于自动请求评审。是否必须获得代码所有者批准由 Branch Protection 或 Ruleset 决定；它不授予路径权限，也不能
代替清晰的模块所有权、人员备份和实际 Review。个人仓库通常不需要建立只有自己一个人的 CODEOWNERS。

## 20. Release、安全与依赖

### 20.1 正式发布链

一个可追溯发布通常形成以下关系：

```text
需求/Issue
  -> 已评审的 Commit
  -> 受保护的 Tag
  -> 固定工具链的 CI 构建
  -> 校验、测试与签名
  -> Release Notes 和 Release Assets
```

Release 工作流模板见 [`templates/github/workflows/release.yml`](templates/github/workflows/release.yml)。模板默认生成 Draft Release，
维护者核对制品和说明后再发布。

### 20.2 固件发布内容

根据产品需要保存：

- `.bin`、`.hex`、`.elf`、map 和调试符号；
- 产品、板卡、硬件版本和构建配置；
- Git Commit、Tag、工具链和 SDK 版本；
- SHA-256 校验和；
- 烧录、升级和回滚步骤；
- 兼容性、已知问题和迁移说明；
- SBOM、签名或 Artifact Attestation。

制品文件名应让使用者在脱离网页后仍能识别，例如：

```text
sensor-controller_board-a_v1.2.0_release.bin
sensor-controller_board-a_v1.2.0_SHA256SUMS.txt
```

### 20.3 Dependabot 与供应链

Dependabot 可以维护 GitHub Actions 和受支持包生态的版本。厂商 SDK、手工 Vendor 代码和未进入包生态的驱动仍需依赖清单与
定期检查。更新 PR 应执行与人工依赖升级相同的构建和测试，不能因为由机器人创建就自动信任。

对正式交付项目，逐步建立：

- 依赖来源、版本、许可证和哈希清单；
- 漏洞报告和修复责任；
- Code Scanning 与秘密扫描；
- 构建来源证明和制品签名；
- 工具链、Action 与基础镜像的受控更新。

### 20.4 凭据泄漏处理

发现秘密进入仓库时：

1. 立即撤销或轮换凭据，不能等待历史清理完成。
2. 确认权限、日志和可能的滥用范围。
3. 从当前代码和构建配置移除秘密，改用 Secrets 或受控配置。
4. 判断仓库是否公开、Commit 是否已被克隆，再决定历史重写范围。
5. 通知所有协作者重新同步被重写的历史。
6. 增加扫描、忽略规则或接口设计，防止再次发生。

已经轮换的旧秘密仍应从公开历史中清理以降低误用和信息暴露，但历史清理本身不能使仍有效的凭据失效。

### 20.5 其他 GitHub 功能的边界

| 功能 | 适合场景 | 使用边界 |
| --- | --- | --- |
| Pages | Doxygen、协议文档、用户手册和静态报告 | 内容需要自动生成或明确维护，不能只复制 README |
| Packages | 可复用库、容器、工具包和有独立消费者的组件 | 单个产品固件通常更适合 Release Asset |
| Wiki | 社区知识和低门槛协作 | 必须与源码同步评审的工程文档放在 `docs/` |
| Environments | 发布、部署和签名等需要审批与秘密隔离的操作 | 不为普通只读 CI 增加无意义关卡 |
| Projects | 多 Issue、路线图和跨仓库计划 | 少量个人 TODO 可以直接由 Issue 或清单管理 |
| Discussions | 问答、公告和开放设计讨论 | 明确可执行工作转为 Issue 追踪 |

仓库只启用有人维护、具有明确入口和退出规则的功能。空 Wiki、失效 Pages 和无人处理的 Discussions 会向使用者提供错误预期。

---

# 第五部分：端到端工作场景

## 21. 个人项目的日常修改

### 21.1 低风险小修改

适用示例：拼写、链接、注释和不影响行为的小型配置说明。

1. 确认 `main` 与远程一致，工作树干净。
2. 完成修改并检查差异。
3. 执行相关检查。
4. 形成一个原子 Commit。
5. 推送后确认远程 CI。

主分支已经要求 PR 时，仍使用 Topic Branch。直接提交只是低风险个人场景的一种简化路径，不改变提交质量要求。

### 21.2 功能或重构

1. 用 Issue 或本地任务说明目标、非目标和验收条件。
2. 从最新 `main` 创建 Topic Branch。
3. 按可验证阶段形成原子 Commit。
4. 推送并创建 Draft PR，使用 Files changed 自查。
5. 运行 CI 和硬件验证，记录结果。
6. 整理说明，选择合适合并方式。
7. 更新本地 `main`，删除完成的分支。

个人 PR 提供一个稳定的自我审查界面，也为未来团队协作积累相同操作经验。

## 22. 团队功能与 Bug 修复

### 22.1 普通功能

1. Issue 明确问题、验收条件、约束和非目标。
2. Topic Branch 只承载一个主要目标。
3. 尽早用 Draft PR 暴露接口和风险。
4. 实现、直接测试和必要文档同步推进。
5. CI 验证全部支持构建，人工或 HIL 记录硬件证据。
6. Reviewer 根据风险检查接口、并发、资源和恢复路径。
7. 处理全部结论并选择仓库允许的合并方式。

### 22.2 Bug 修复

1. 记录版本、硬件、环境、复现和最后正常版本。
2. 尽可能建立失败测试、复现脚本或确定的硬件步骤。
3. 查明根因和影响边界，避免只隐藏症状。
4. 检查相邻错误路径与回归风险。
5. PR 说明根因、修复原理、验证证据和兼容影响。
6. 已发布版本通过新 PATCH 或项目定义的维护版本交付。

## 23. 实验、发布与紧急恢复

### 23.1 风险实验

使用 `spike/...` 分支，并记录假设、时间边界和成功条件。允许临时代码，但不允许真实凭据、来源不明文件和会污染正式
接口的隐式依赖。实验成功后整理为可维护的正式变更；失败后记录结论并关闭分支。

### 23.2 正式发布

1. 确认目标版本范围和已知问题。
2. `main` 的要求检查全部通过。
3. 更新版本源、Changelog 和迁移说明。
4. 验证支持硬件、升级、存储兼容和回滚路径。
5. 创建受保护 Tag，CI 从 Tag 构建。
6. 生成校验和、元数据和所需安全证明。
7. 核对 Draft Release 的制品、硬件范围与说明。
8. 发布稳定版或 prerelease，保留不可变来源关系。

### 23.3 紧急恢复

先区分三个动作：停止分发有问题的版本、让设备回到可用版本、在源码历史中回退修改。它们可能需要不同权限和验证。

共享源码使用 Revert PR 保留审计；设备回滚需要确认 Bootloader、参数和存储格式兼容；修复结果创建新版本。安全问题还需评估
通知、凭据轮换和受影响设备范围。

---

# 第六部分：检查清单与速查

## 24. 新仓库检查清单

- [ ] README 能让新环境完成最小构建、烧录和验证。
- [ ] 工具链、SDK、生成器和硬件版本已记录。
- [ ] 手写源码、配置源、生成文件、第三方代码和构建输出边界清楚。
- [ ] `.gitignore` 基于真实输出，`.gitattributes` 明确文本和行尾。
- [ ] 构建与测试具有非交互入口和可靠退出码。
- [ ] 第三方组件具有来源、版本、许可证和修改记录。
- [ ] 仓库不包含秘密、个人绝对路径和意外大文件。
- [ ] 主分支含义、合并方式和版本方法已说明。
- [ ] GitHub 模板和 CI 只启用项目能够维护的部分。

## 25. Commit 检查清单

- [ ] 当前分支和远程基线正确。
- [ ] Commit 只有一个可以清楚命名的主要目的。
- [ ] 已分别检查工作树和暂存区。
- [ ] 没有无关格式化、生成噪声、日志、凭据和构建输出。
- [ ] 生成配置与需要提交的生成结果一致。
- [ ] 已执行与风险相称的构建、分析和测试。
- [ ] 标题说明结果，正文解释必要动机和边界。

## 26. PR 检查清单

- [ ] Base 与 Compare 分支正确，PR 关联目标 Issue。
- [ ] 标题、目标、非目标和主要变化清楚。
- [ ] Files changed 不包含无关修改。
- [ ] CI 通过，失败、跳过和基础设施问题均有解释。
- [ ] 硬件目标、版本、配置、步骤和结果已记录。
- [ ] 已评估中断、并发、超时、恢复、内存、时序和兼容影响。
- [ ] 文档、协议、配置和 Changelog 已按需要同步。
- [ ] Review 结论全部处理，合并方式符合仓库设置。

## 27. Release 检查清单

- [ ] 版本号与已声明兼容面匹配。
- [ ] Tag 指向通过验证的 Commit，版本源一致。
- [ ] 正式制品由固定工具链从 Tag 构建。
- [ ] 文件名、硬件范围、校验和和构建元数据完整。
- [ ] Release Notes 说明新增、修复、兼容性、升级和已知问题。
- [ ] 已验证烧录、升级、存储迁移和回滚。
- [ ] 稳定版与 prerelease 标记正确。
- [ ] 公开后不移动 Tag 或覆盖同名制品。

## 28. Git 命令速查

| 目的 | 命令 |
| --- | --- |
| 查看状态 | `git status --short --branch` |
| 查看未暂存差异 | `git diff` |
| 查看已暂存差异 | `git diff --cached` |
| 检查空白错误 | `git diff --check`、`git diff --cached --check` |
| 创建并切换分支 | `git switch -c <branch>` |
| 更新远程信息 | `git fetch --prune origin` |
| 快进本地主分支 | `git merge --ff-only origin/main` |
| 取消暂存 | `git restore --staged <path>` |
| 放弃工作树修改 | `git restore <path>` |
| 回退共享 Commit | `git revert <commit>` |
| 把指定 Commit 应用到当前分支 | `git cherry-pick <commit>` |
| 查看提交图 | `git log --oneline --decorate --graph --all` |
| 查找丢失 Commit | `git reflog` |
| 安全更新已重写分支 | `git push --force-with-lease` |
| 预览未跟踪文件清理 | `git clean -nd` |
| 创建带说明 Tag | `git tag -a <tag> -m <message>` |

任何会丢失修改或改写历史的命令，都应先执行 `status`、查看相关差异，并判断历史是否已经共享。

## 29. GitHub 功能选择表

| 需求 | 首选功能 | 说明 |
| --- | --- | --- |
| 记录可执行问题 | Issue | 包含背景、约束和验收条件 |
| 评审并合入分支 | Pull Request | 集中差异、讨论、Review 和 Checks |
| 自动构建与测试 | Actions | 调用可在本地复现的命令 |
| 保护主分支和 Tag | Rulesets | 从实际稳定检查逐步启用 |
| 发布版本和正式制品 | Releases | 与 Tag、版本说明和校验和关联 |
| 保存一次 CI 运行结果 | Workflow Artifacts | 有保留期，不等同正式 Release |
| 管理多任务路线图 | Projects | 小型 TODO 无需引入 |
| 开放问答和设计讨论 | Discussions | 可执行结论再转 Issue |

## 30. 权威资料

本指南的工具行为和平台能力以以下官方资料为依据：

- [Git Reference](https://git-scm.com/docs)
- [Git Workflows](https://git-scm.com/docs/gitworkflows)
- [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/)
- [Semantic Versioning 2.0.0](https://semver.org/)
- [Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/)
- [GitHub：About pull requests](https://docs.github.com/en/pull-requests/get-started/about-pull-requests)
- [GitHub：About merge methods](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/configuring-pull-request-merges/about-merge-methods-on-github)
- [GitHub：About rulesets](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/about-rulesets)
- [GitHub：Commit signature verification](https://docs.github.com/en/authentication/managing-commit-signature-verification/about-commit-signature-verification)
- [GitHub：Issue and pull request templates](https://docs.github.com/en/communities/using-templates-to-encourage-useful-issues-and-pull-requests)
- [GitHub：About releases](https://docs.github.com/en/repositories/releasing-projects-on-github/about-releases)
- [GitHub：Workflow artifacts](https://docs.github.com/en/actions/concepts/workflows-and-actions/workflow-artifacts)
- [GitHub Actions：Secure use reference](https://docs.github.com/en/actions/reference/security/secure-use)
- [GitHub：Self-hosted runners](https://docs.github.com/en/actions/reference/runners/self-hosted-runners)
- [GitHub：Artifact attestations](https://docs.github.com/en/actions/concepts/security/artifact-attestations)

GitHub 的界面、功能名称、套餐范围和安全建议会变化。维护本指南时，应复核官方资料并更新首页的复核日期。

## 31. 维护与扩展

本文采用语义化版本：

- PATCH：纠错、链接更新、命令说明和不改变整体结构的补充；
- MINOR：增加新的完整主题、工程场景或模板；
- MAJOR：改变文档定位、核心工作流模型或适用范围。

未来可独立扩展以下主题，同时保持现有章节稳定：

- Git Bisect、Worktree、Submodule 与大型仓库维护；
- 多仓库版本协调和组件发布；
- 嵌入式 Linux、Yocto 与 Buildroot 仓库模型；
- 完整 SBOM、签名、供应链等级和法规交付；
- 大规模团队的 Merge Queue、CODEOWNERS 和发布列车。

新增内容应说明解决的问题、适用条件、操作方法、失败模式和验证方式，避免只罗列工具功能。

## Changelog

### v1.0.0 — 2026-07-24

- 建立工程仓库、Git 和 GitHub 的渐进式学习路径。
- 明确物理仓库分类，并引用独立的代码风格和架构规范。
- 从 Git 数据模型开始完整讲解日常提交、分支、同步、恢复和版本管理。
- 增加从 Issue 到 Pull Request、Review、CI、合并和 Release 的端到端实践。
- 增加 Actions 安全、Rulesets、HIL、依赖和正式制品的进阶说明。
- 将可复用 README、Issue、PR、CI 和 Release 内容维护为独立模板集。
