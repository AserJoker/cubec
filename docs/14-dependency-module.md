# Cubec 依赖与模块系统设计

> **重要状态说明**：本文档描述的是**规划中的**依赖与模块系统设计。基于当前代码
> （`src/main.c`、`src/cmd/`、CMakeLists.txt）核查，**该依赖管理系统尚未实现**：
> - 编译器 CLI 入口（`src/main.c`）目前仅注册了 **`format`** 一个子命令
>   （`cmd/format.c`，代码格式化），**没有** `build` / `test` / `run` / `fetch` 子命令
> - 代码中**不存在** `cubec.json` 解析、`library/` 目录管理、`CUBEC_HOME`、
>   `fetch` 等实现（`src/core/env.c`、`src/core/allocator.c` 仅含通用环境/分配器辅助）
> - 模块导入（`vm_import`）目前只做路径解析与 AST 加载，未接入项目级依赖解析
>
> 因此，本文档作为**设计目标**保留，与已实现代码的偏差如下方"当前实现状态"小节所述。

## 1. 总体原则（设计目标）

Cubec 计划采用**去中心化**依赖管理，依赖来源可以是任何支持 Git 协议的远程仓库（如 GitHub），或本地文件路径（`file:///D:/projects/xxxxx`）。

Cubec 计划支持两种模式：
- **单文件模式**：直接操作文件名
- **项目模式**：自动读取目录下的 `cubec.json`，无需指定文件名

## 2. 项目描述文件 cubec.json（设计）

> 当前代码未实现 cubec.json 解析。

项目根目录下存在 `cubec.json` 即视为 Cubec 项目，格式如下：

```json
{
  "name": "my_project",
  "deps": {
    "collections": {
      "url": "https://github.com/cubec-lang/collections",
      "branch": "main",
      "md5": "a1b2c3d4..."
    }
  },
  "replace": {
    "utils@main": "utils@v2"
  },
  "index": "src/index.cubec",
  "build": "src/build.cubec"
}
```

### 字段说明

| 字段 | 必填 | 说明 |
|------|------|------|
| `name` | 是 | 项目名称，作为依赖引入时的标识符 |
| `deps` | 否 | 依赖声明，key 为依赖名，value 为来源描述 |
| `deps.<name>.url` | 是 | Git 仓库地址或 `file://` 本地路径 |
| `deps.<name>.branch` | 否 | Git 分支或 tag，默认为默认分支 |
| `deps.<name>.md5` | 否 | 仓库内容的校验哈希，用于完整性验证 |
| `replace` | 否 | 依赖替换映射，格式 `"name@branch": "replacement_name@branch"` |
| `index` | 否 | 入口文件路径。缺失则此项目不可作为依赖引入 |
| `build` | 否 | 构建脚本路径。缺失则此项目不可单独编译（仅源码引入） |

### 版本管理策略

版本由 Git tag/branch 管理，不使用独立的 `version` 字段。依赖版本通过 `deps.<name>.branch` 指定 Git 分支或 tag，md5 可选用于校验完整性。

## 3. 依赖解析（设计）

### 3.1 import 路径语法

| 语法 | 含义 | 解析位置 |
|------|------|----------|
| `import x from "./foo"` | 相对路径 | 相对于当前文件的目录 |
| `import x from "std/io"` | 标准库 | `${CUBEC_HOME}/library/std/io/index.cubec` |
| `import x from "collections/vec"` | 项目依赖 | `${PROJECT}/library/collections/vec/index.cubec` |
| `import x from "collections/vec"` | 全局依赖（无 manifest） | `${CUBEC_HOME}/library/collections/vec/index.cubec` |

**关键规则**：
- `std` 前缀保留给标准库，自定义依赖**禁止**命名为 `std`
- 非相对路径（无 `./` 或 `../` 前缀）按依赖名前缀解析
- 路径映射到目录时，自动加载该目录下的 `index.cubec`

### 3.2 解析优先级

1. `std` 前缀 → `${CUBEC_HOME}/library/std/`
2. 项目依赖（存在 cubec.json）→ 必须在 `deps` 中声明才可 import，从 `${PROJECT}/library/<dep_name>/` 解析。未声明的依赖即使目录存在也不可引用（报编译错误）
3. 全局依赖 → `${CUBEC_HOME}/library/<dep_name>/`（无 cubec.json 时回退）
4. **幽灵依赖禁止**：只能 import cubec.json 中声明的依赖，不允许 import 间接依赖

### 3.3 依赖存放位置

- **项目依赖**：`${PROJECT}/library/<dep_name>/`
- **标准库**：`${CUBEC_HOME}/library/std/`
- **全局依赖**：`${CUBEC_HOME}/library/<dep_name>/`
- **开发阶段**：`CUBEC_HOME` 默认为当前项目根目录
- **配置方式**：环境变量 `CUBEC_HOME` 为默认值，可通过 CLI `--home` 参数覆盖

### 3.4 依赖扁平化

所有依赖（包括间接依赖）递归展开下载到工作项目的 `library/` 下。Cubec 要求依赖**扁平化管理**，但通过 cubec.json 声明限制，防止幽灵依赖。

### 3.5 循环依赖

**允许循环依赖**，编译器通过延迟绑定（模块缓存 + 状态机）处理，与当前模块内的循环引用处理方式一致（见 `15-name-collection.md` 第 5.2 节）。

## 4. 依赖获取（设计）

规划中提供 `cubec fetch` 命令显式获取依赖（编译时不自动获取，依赖缺失则报错）：
- 解析 cubec.json 中的 deps
- 递归获取所有间接依赖
- 将依赖 clone 到 `library/` 目录下
- 如已存在且 md5 匹配则跳过

命令：`cubec fetch`（必须手动执行，编译前获取依赖）

## 5. 依赖冲突与 replace（设计）

间接依赖递归展开时可能出现版本冲突。用户通过 `replace` 字段主动解决：

```json
"replace": {
  "utils@main": "utils@v2"
}
```

语义：将所有对 `utils@main` 的依赖替换为 `utils@v2`（替换依赖源）。具体代码不兼容问题由用户自行解决。

## 6. export 扩展语法（设计）

### 6.1 代理导出（re-export）

```cubec
// 全量重新导出
export * from "./math";

// 选择性重新导出
export { add, subtract } from "./math";
```

### 6.2 index.cubec 示例

```cubec
// index.cubec — 项目入口，统一导出所有公开 API
export * from "./vec";
export * from "./map";
export { HashMap, HashSet } from "./hash";
```

## 7. 单文件模式（设计）

单文件模式下：
- 可 `import` 依赖（从 `${CUBEC_HOME}/library/` 解析）
- 不可 `export`（本身不能作为依赖引入）
- 无 cubec.json，无项目级依赖管理

## 8. 标准库（设计）

- 标准库位于 `${CUBEC_HOME}/library/std/`
- `std` 命名空间保留，自定义依赖禁止覆盖
- 开发阶段，`CUBEC_HOME` 默认为当前项目根目录
- 标准库首批模块：`std/io`、`std/str`、`std/result`、`std/collections`

## 9. CLI 命令总览（规划）

| 命令 | 说明 | 当前状态 |
|------|------|----------|
| `cubec build test.cubec` | 单文件构建 | ❌ 未实现 |
| `cubec test test.cubec` | 单文件测试 | ❌ 未实现 |
| `cubec run test.cubec` | 单文件运行 | ❌ 未实现 |
| `cubec build` | 项目构建（读取 cubec.json） | ❌ 未实现 |
| `cubec test` | 项目测试 | ❌ 未实现 |
| `cubec fetch` | 获取项目依赖（手动） | ❌ 未实现 |
| `cubec format` | 代码格式化 | ✅ 已实现（`src/cmd/format.c`） |
| `cubec build --home /path` | 指定 CUBEC_HOME | ❌ 未实现 |

## 10. 待设计/后续事项

- [ ] build 脚本（`build.cubec`）的详细 API 设计
- [ ] `cubec fetch` 的完整解析算法（依赖图构建、冲突检测）
- [ ] md5 校验的计算方式（整个仓库内容 vs 特定文件）
- [ ] `file://` 本地依赖的更新检测策略
- [ ] `CUBEC_HOME` 的配置方式（环境变量 + CLI `--home` 参数）
- [ ] CLI 子命令框架（当前仅 `format`，需扩展 build/test/run/fetch）

## 11. 当前实现状态（与代码的偏差）

- **CLI**：`src/main.c` 仅注册 `cmd_format` 子命令；`src/cmd/` 仅有 `format.c`
- **依赖解析**：`vm_import`（`src/engine/vm.c`）做路径解析 + AST 加载，但未接入
  cubec.json / library / CUBEC_HOME
- **代码生成**：C 后端为 `src/cubec/` 的分发式 `emit_*`（见 `11-codegen.md`），
  尚无统一的 build/run 驱动
- 模块系统的核心机制（名字收集、循环依赖延迟绑定、export/import）已在
  `15-name-collection.md` 描述并部分实现，但其上是项目级依赖管理尚未落地
