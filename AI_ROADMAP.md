# Profex AI-Agent 友好化 + MCP 支持 — 总计划

**仓库**: https://github.com/PhaseAnalysisXRD/profex
**上游**: https://www.profex-xrd.org/ (Version 5.6.1)
**核心原则**: 保持核心源码不变，所有修改只在外围添加

---

## Phase 1: 项目基础建设 ✅ (已完成)

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 1.1 | LICENSE 文件 | ✅ | GPL v2 全文 |
| 1.2 | README.md | ✅ | 项目说明、构建指南、AI 路线图、MCP 规划 |
| 1.3 | .gitignore | ✅ | Qt/C++ 项目专用 |
| 1.4 | 核心 API Doxygen 注释 | ✅ | scan.h, functions.h, structs.h + mainwindow.h + 6 个关键类 |
| 1.5 | Doxyfile + 文档生成 | ✅ | 297 页 HTML，覆盖率 82 个类 |
| 1.6 | Qt5 兼容性修复 | ✅ | 修正 17 处 Qt5 编译错误（QKeyCombination, setEncoding, first() 等） |
| 1.7 | MCP 基础结构 | ✅ | .mcp.json + mcp-server/README.md |

---

## Phase 2: 构建脚本与开发环境 ✅ (已完成)

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 2.1 | `scripts/build.sh` | ✅ | 一键构建脚本，自动检测 Qt 版本，支持 qmake/cmake |
| 2.2 | `scripts/setup-dev.sh` | ✅ | 开发环境一键配置 (Debian/Ubuntu/Fedora/macOS) |
| 2.3 | CMakeLists.txt (项目级) | ⬜ | 顶层 CMake 构建支持 |
| 2.4 | GitHub Actions CI | ✅ | 自动构建测试 + Doxygen 文档生成 |
| 2.5 | `.clangd` 配置 | ✅ | AI code editor 索引配置 |

---

## Phase 3: Headless CLI / Python 桥接 (当前)

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 3.1 | Python pybind11 绑定 | ⬜ | 暴露核心 XRD 计算函数到 Python |
| 3.2 | JSON 结构化输出格式 | ⬜ | 精修结果标准 JSON schema |
| 3.3 | Headless 运行模式 | ⬜ | Profex 命令行模式，无需 GUI |
| 3.4 | BGMN 参数 YAML 配置 | ⬜ | 使用 YAML 而非 BGMN 原生格式配置 |

---

## Phase 4: MCP Server 实现

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 4.1 | MCP Server 框架搭建 | ⬜ | Python MCP SDK 基础服务 |
| 4.2 | Tool: `run_refinement` | ⬜ | 执行 BGMN 精修 |
| 4.3 | Tool: `get_phase_results` | ⬜ | 获取物相定量结果 |
| 4.4 | Tool: `search_phase` | ⬜ | 按名称/化学式搜索物相 |
| 4.5 | Tool: `list_scans` | ⬜ | 列出已加载的 XRD 扫描 |
| 4.6 | Resource: `profex://project/{id}/phases` | ⬜ | 物相数据结构化访问 |
| 4.7 | Resource: `profex://project/{id}/report` | ⬜ | 精修报告访问 |
| 4.8 | AI Editor MCP 集成文档 | ⬜ | 配置示例（VS Code, Cursor, Claude 等） |

---

## Phase 5: AI Agent 深层集成

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 5.1 | AI 驱动的参数优化 | ⬜ | LLM 自动优化精修参数 |
| 5.2 | 自动物相鉴定助手 | ⬜ | AI 辅助 Search-Match |
| 5.3 | RAG 知识库 | ⬜ | XRD/Profex 文档语义搜索 |
| 5.4 | 批量处理脚本 | ⬜ | AI 驱动的批量精修编排 |

---

## 阶段完成记录

每个 Phase 完成后在此更新完成报告。
