# Profex-MCP

**Headless MCP servers for Profex / BGMN XRD analysis**

[![GPLv2 License](https://img.shields.io/badge/License-GPLv2-blue.svg)](LICENSE)
[![Python 3.10+](https://img.shields.io/badge/Python-3.10+-blue.svg)]()
[![MCP Protocol](https://img.shields.io/badge/MCP-1.0+-green.svg)](https://modelcontextprotocol.io)

Profex-MCP 为经典粉末 XRD 分析工具 [Profex](https://www.profex-xrd.org/) 和 [BGMN](https://www.bgmn.de/) 提供了 AI 友好的 MCP 接口。通过两组 MCP 服务器，AI 智能体可以:

- 🔬 **物相鉴定 (Search-Match)** — 从衍射峰位自动识别物相
- 📡 **COD 数据库查询** — 在线搜索 COD 晶体学数据库
- ⚙️ **BGMN 精修控制** — 执行、监控、批量精修
- 🧪 **数据格式转换** — XRD 数据文件格式互相转换
- 🤖 **AI 增强分析** — 精修顾问、搜索匹配工作流

---

## 项目结构 / Structure

```
Profex-MCP/
├── pyprofex/                          # ✅ Python MCP 服务器 + 分析引擎
│   ├── mcp_server.py                  # MCP Server: XRD 分析 (11 tools)
│   ├── mcp_server_refine.py           # MCP Server: BGMN 精修 (4 tools)
│   ├── search_match.py                # Search-Match 引擎 (迭代扣除 + 独立评分)
│   ├── peaks.py                       # 寻峰算法 (高斯拟合)
│   ├── fingerprints.py                # 手动验证指纹库 (27 矿物)
│   ├── fingerprints_auto.py           # CIF 自动生成指纹库 (~2260 矿物)
│   ├── fingerprints_unified.pkl       # 🔥 预编译统一指纹库 (2289 矿物, 0.02s 加载)
│   ├── profex_cli.py                  # CLI 入口
│   ├── build_fingerprints.py          # COD db3 → 指纹库构建工具
│   ├── build_unified_db.py            # 三个指纹源 → 统一 pickle
│   ├── build_powcod_fingerprints.py   # Qualx2 POWCOD 数据库转换
│   ├── cif2fingerprint.py             # CIF → 理论衍射花样计算引擎
│   ├── tests/                         # pytest 测试集
│   └── pyproject.toml                 # Python 包配置
├── profex/                            # Profex C++ 源码 (Qt5, 未修改)
├── cmdtools/                          # Profex 命令行工具源码
├── modules/                           # 可选的 Profex 模块
├── mcp-server/                        # MCP 集成文档
├── knowledge-base/                    # XRD/Rietveld/BGMN RAG 知识库
├── .mcp.json                          # MCP 宿主集成配置
├── AI_ROADMAP.md                      # 完整开发路线图
└── SEARCH_MATCH_ROADMAP.md            # Search-Match 优化路线图
```

---

## 快速开始 / Quick Start

### 前置要求

- **Python 3.10+**
- **Profex + BGMN** 二进制文件（可选 — 用于精修工具）

### 1️⃣ 安装 Python 依赖

```bash
git clone https://github.com/XRDFPSR/Profex-MCP.git
cd Profex-MCP

pip install -r pyprofex/requirements.txt 2>/dev/null || \
pip install numpy scipy mcp>=1.0
```

核心依赖：
- `mcp>=1.0` — MCP 协议框架
- `numpy` — 数值计算
- `scipy` — 寻峰 (find_peaks_cwt)

### 2️⃣ 验证安装

```bash
# 验证 Search-Match 引擎
python3 -c "from pyprofex.search_match import get_db; db = get_db(); print(f'{len(db)} fingerprints loaded')"

# 验证 MCP Server 导入
python3 -c "from pyprofex.mcp_server import server; print('mcp_server OK')"
from pyprofex.mcp_server_refine import server_refine; print('mcp_server_refine OK')"
```

### 3️⃣ 启动 MCP 服务器

```bash
# XRD 分析 (Search-Match + COD + 数据转换)
python3 pyprofex/mcp_server.py

# BGMN 精修
python3 pyprofex/mcp_server_refine.py
```

---

## MCP 工具清单

### XRD 分析 (`mcp_server.py`) — 11 个工具

| 工具 | 功能 |
|------|------|
| `list_projects` | 列示 Profex 项目文件 |
| `convert_file` | XRD 数据格式转换 |
| `parse_parameters` | 解析 BGMN .par 文件 |
| `list_scans` | 列示 XRD 扫描数据 |
| `available_formats` | 列出支持的格式 |
| `search_phase` | 搜索物相结构文件 |
| **`identify_phases`** | 🔥 **核心工具**: 从 d-spacing 列表自动识别物相 (Search-Match) |
| `cod_search` | COD 数据库搜索 |
| `cod_get_cif` | 下载 COD CIF 文件 |
| `cod_search_by_d` | 按 d-spacing 搜索 COD |
| **`search_match`** | 🔥 **端到端管线**: 加载 XRD 数据 → 寻峰 → Search-Match |

### 精修控制 (`mcp_server_refine.py`) — 4 个工具

| 工具 | 功能 |
|------|------|
| `run_refinement` | 通过 .sav 控制文件执行 BGMN 精修 |
| `get_results` | 解析 .par 精修结果 (Rwp, GoF, 物相定量) |
| `batch_refine` | 多参数组合自动批量精修 |
| `list_sav_files` | 列示 .sav 控制文件 |

### Prompts (提示词模版)

| 提示词 | 功能 |
|--------|------|
| `refine_advisor` | AI 精修顾问 — 参数选择和问题诊断 |
| `search_match_workflow` | 引导式 Search-Match 工作流 |

---

## Search-Match 引擎

核心 Search-Match 引擎位于 `search_match.py`，支持：

### 算法特性

- **多源指纹库**：手动验证库 (27) + CIF 自动生成库 (~2260) = **2289 条指纹**
- **快速加载**：预编译 pickle 加载仅需 **0.02s**
- **迭代扣除算法**：逐相匹配 → 扣除 → 迭代，支持多相混合样品
- **独立评分算法**：对所有候选独立评分，推荐最优
- **稀有度加权评分**：对匹配到"稀有"峰（少数候选能匹配的峰）的物相加分
- **强度加权 FOM**：强峰的匹配权重更高
- **择优取向检测**：标记可能的择优取向效应
- **元素过滤**：结合 EDX/EDS 元素信息提升准确度

### 评分指标

| 指标 | 说明 |
|------|------|
| `matches` | 匹配的理论峰数 |
| `intensity_coverage` | 强度加权覆盖率 (0-1) |
| `fom` | 综合置信度 (matches × intensity / unmatched) |
| `element_coverage` | 元素覆盖率 |
| `preferred_orientation` | 择优取向标记 |

### 使用示例

```python
from pyprofex.search_match import iterative_search_match

d_spacings = [3.34, 2.46, 1.82, 1.54, 1.38]  # Quartz
result = iterative_search_match(d_spacings, ['Si', 'O'])
for p in result['phases']:
    print(f"{p['name']}: matches={p['matches']}, FOM={p['fom']}")
```

---

## COD 数据库接口

集成 [Crystallography Open Database (COD)](http://www.crystallography.net/) 在线 API：

| 查询 | 示例 |
|------|------|
| 按元素 | `cod_search("Fe,O")` → 含 Fe 和 O 的物相 |
| 按矿物名 | `cod_search("Quartz")` → 石英条目 |
| 按化学式 | `cod_search("SiO2")` → 二氧化硅条目 |
| 按 COD ID | `cod_search("1011097")` → 下载指定 CIF |
| d-spacing 匹配 | `cod_search_by_d("3.34,2.46,1.82")` → d 值匹配 |

---

## 项目路线图 / Roadmap

所有 Phase 1-6 开发阶段已完成 ✅。[查看完整路线图](AI_ROADMAP.md)

| 阶段 | 内容 | 状态 |
|------|------|------|
| Phase 1 | 项目基础建设 (LICENSE, README, .gitignore, Doxygen, Qt5 兼容) | ✅ |
| Phase 2 | 构建脚本与 CI (build.sh, setup-dev.sh, GitHub Actions) | ✅ |
| Phase 3 | Headless CLI + Python 桥接 (profex_cli.py, JSON schema, YAML) | ✅ |
| Phase 4 | MCP Server (11 个工具, 资源, 提示词, AI Editor 集成) | ✅ |
| Phase 5 | AI 深层集成 (精修控制, 批量精修, 报告生成, XRD RAG) | ✅ |
| Phase 6 | COD 数据库 API (搜索, CIF 下载, d-spacing 匹配) | ✅ |
| Phase S0-S3 | Search-Match 优化 (迭代扣除, 自适应容差, 非晶检测) | ✅ |
| — | **统一指纹库 pickle 构建** | ✅ |

### 下一步

- [ ] 扩展指纹库到全部 COD (530K 条目)
- [ ] 增加 PXRD 数据自动预处理管线
- [ ] CI/CD (GitHub Actions 自动化测试)
- [ ] Docker 镜像（含 Profex + BGMN 预装）

---

## 衍生说明 / Derivation

本项目源自 [Profex](https://www.profex-xrd.org/) (GPLv2+)，由 Nico B. 和同事们开发。Profex 是一款基于 Qt5 的 BGMN GUI 界面，用于粉末 XRD 的 Rietveld 精修。

**Profex-MCP 在此基础上增加了：**

1. **MCP 服务器** — 通过 stdio/SSE 协议暴露 Profex/BGMN 功能
2. **Python 桥接** — Search-Match、数据格式转换、物相解析
3. **COD 数据库集成** — 在线 API 和 2289 矿物指纹库
4. **AI 工作流** — 精修顾问、Search-Match 引导、迭代扣除算法
5. **RAG 知识库** — BGMN 参数、Rietveld 原理、物相鉴定策略

---

## 许可证 / License

**GNU General Public License v2.0 or later (GPL-2.0-or-later)**
