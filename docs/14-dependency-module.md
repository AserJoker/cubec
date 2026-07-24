# Cubec 依赖与模块系统设计

## 1. 总体原则

Cubec 采用**去中心化**依赖管理，依赖来源可以是任何支持 Git 协议的远程仓库（如 GitHub），或本地文件路径（`file:///D:/projects/xxxxx`）。

Cubec 支持两种模式：
- **单文件模式**：直接操作文件名，如 `cubec build test.cubec`、`cubec test test.cubec`、`cubec run test.cubec`
- **项目模式**：自动读取目录下的 `manifest.json`，无需指定文件名

## 2. 项目描述文件 manifest.json

项目根目录下存在 `manifest.json` 即视为 Cubec 项目，格式如下：

```json
{
  "name": "my_project",
  "version": "0.1.0",
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
| `version` | 是 | 项目版本号 |
| `deps` | 否 | 依赖声明，key 为依赖名，value 为来源描述 |
| `deps.<name>.url` | 是 | Git 仓库地址或 `file://` 本地路径 |
| `deps.<name>.branch` | 否 | Git 分支或 tag，默认为默认分支 |
| `deps.<name>.md5` | 否 | 仓库内容的校验哈希，用于完整性验证 |
| `replace` | 否 | 依赖替换映射，格式 `"name@branch": "replacement_name@branch"` |
| `index` | 否 | 入口文件路径。缺失则此项目不可作为依赖引入 |
| `build` | 否 | 构建脚本路径。缺失则此项目不可单独编译（仅源码引入） |

### 版本管理策略

采用 **Git branch/tag + md5 锁定**，不使用 semver 范围解析。用户通过 branch/tag 指定版本，md5 可选用于校验完整性。

## 3. 依赖解析

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
2. 项目依赖（存在 manifest.json）→ 必须在 `deps` 中声明才可 import，从 `${PROJECT}/library/<dep_name>/` 解析。未声明的依赖即使目录存在也不可引用（报编译错误）
3. 全局依赖 → `${CUBEC_HOME}/library/<dep_name>/`（无 manifest.json 时回退）
4. **幽灵依赖禁止**：只能 import manifest.json 中声明的依赖，不允许 import 间接依赖

### 3.3 依赖存放位置

- **项目依赖**：`${PROJECT}/library/<dep_name>/`
- **标准库**：`${CUBEC_HOME}/library/std/`
- **全局依赖**：`${CUBEC_HOME}/library/<dep_name>/`
- **开发阶段**：`CUBEC_HOME` 默认为当前项目根目录
- **配置方式**：环境变量 `CUBEC_HOME` 为默认值，可通过 CLI `--home` 参数覆盖

### 3.4 依赖扁平化

所有依赖（包括间接依赖）递归展开下载到工作项目的 `library/` 下。Cubec 要求依赖**扁平化管理**，但通过 manifest.json 声明限制，防止幽灵依赖。

### 3.5 循环依赖

**允许循环依赖**，编译器通过延迟绑定（TDZ）处理，与当前模块内的循环引用处理方式一致。

## 4. 依赖获取

Cubec 编译器提供 `cubec fetch` 命令显式获取依赖（编译时不自动获取，依赖缺失则报错）：
- 解析 manifest.json 中的 deps
- 递归获取所有间接依赖
- 将依赖 clone 到 `library/` 目录下
- 如已存在且 md5 匹配则跳过

命令：`cubec fetch`（必须手动执行，编译前获取依赖）

## 5. 依赖冲突与 replace

间接依赖递归展开时可能出现版本冲突。用户通过 `replace` 字段主动解决：

```json
"replace": {
  "utils@main": "utils@v2"
}
```

语义：将所有对 `utils@main` 的依赖替换为 `utils@v2`（替换依赖源）。具体代码不兼容问题由用户自行解决。

## 6. export 扩展语法

### 6.1 代理导出（re-export）

为支持 `index.cubec` 统一导出项目模块，新增两种语法：

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

## 7. 单文件模式

单文件模式下：
- 可 `import` 依赖（从 `${CUBEC_HOME}/library/` 解析）
- 不可 `export`（本身不能作为依赖引入）
- 无 manifest.json，无项目级依赖管理

## 8. 标准库

- 标准库位于 `${CUBEC_HOME}/library/std/`
- `std` 命名空间保留，自定义依赖禁止覆盖
- 开发阶段，`CUBEC_HOME` 默认为当前项目根目录
- 标准库首批模块：`std/io`、`std/str`、`std/result`、`std/collections`

## 9. CLI 命令总览

| 命令 | 说明 |
|------|------|
| `cubec build test.cubec` | 单文件构建 |
| `cubec test test.cubec` | 单文件测试 |
| `cubec run test.cubec` | 单文件运行 |
| `cubec build` | 项目构建（读取 manifest.json） |
| `cubec test` | 项目测试 |
| `cubec fetch` | 获取项目依赖（手动） |
| `cubec build --home /path` | 指定 CUBEC_HOME |

## 10. 待设计/后续事项

- [ ] build 脚本（`build.cubec`）的详细 API 设计
- [ ] `cubec fetch` 的完整解析算法（依赖图构建、冲突检测）
- [ ] md5 校验的计算方式（整个仓库内容 vs 特定文件）
- [ ] `file://` 本地依赖的更新检测策略
- [ ] `CUBEC_HOME` 的配置方式（环境变量 + CLI `--home` 参数，已确定）
