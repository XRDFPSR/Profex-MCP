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

## Phase 3: Headless CLI / Python 桥接 ✅ (已完成)

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 3.1 | Python 包 — pyprofex | ✅ | XRD 计算 + BGMN .par 解析 + ProfexCLI 封装 |
| 3.2 | JSON 结构化输出格式 | ✅ | JSON Schema 完整定义 (phases, Rwp, GoF, cell params) |
| 3.3 | Headless CLI 模式 | ✅ | profex_cli.py (convert/info/parse-params/results) + --json 输出 |
| 3.4 | BGMN 参数 YAML 配置 | ✅ | YAML 替代 raw BGMN 格式的完整示例 |

---

## Phase 4: MCP Server 实现 (当前)

---

## Phase 4: MCP Server 实现 ✅ (已完成)

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 4.1 | MCP Server 框架搭建 | ✅ | Python MCP SDK 1.x 完整服务 |
| 4.2 | Tool: list_projects / convert_file / parse_parameters | ✅ | 项目管理 + 文件转换 + 参数解析 |
| 4.3 | Tool: list_scans / available_formats / search_phase | ✅ | 扫描列示 + 格式查询 + 物相搜索 |
| 4.4 | Resource: profex:// 协议 | ✅ | project/params/info 三类资源 |
| 4.5 | Prompt: analyze_xrd | ✅ | 引导式 XRD 分析工作流 |
| 4.6 | AI Editor 集成文档 | ✅ | VS Code / Claude / Cursor 配置指南 |
| 4.7 | .mcp.json 更新 | ✅ | 添加 profex-xrd MCP server 入口 |

---

## Phase 5: AI Agent 深层集成 ✅ (已完成)

| # | 任务 | 状态 | 说明 |
|---|------|------|------|
| 5.1 | **MCP Tool: run_refinement** | ✅ | 通过 subprocess 调用 BGMN 精修引擎 |
| 5.2 | **MCP Tool: get_results** | ✅ | .par 解析提取 Rwp/GoF/物相定量 |
| 5.3 | **MCP Tool: batch_refine** | ✅ | 多参数组合自动批量精修 |
| 5.4 | **MCP Tool: list_sav_files** | ✅ | .sav 控制文件发现 |
| 5.5 | **AI Search-Match 助手** | ✅ | COD 数据库物相鉴定工作流 (prompt) |
| 5.6 | **XRD RAG 知识库** | ✅ | bgmn-parameters + rietveld + phase-ID |
| 5.7 | **Refinement Advisor Prompt** | ✅ | AI 精修顾问 — 参数选择和问题诊断 |

---

## 阶段完成记录

每个 Phase 完成后在此更新完成报告。
