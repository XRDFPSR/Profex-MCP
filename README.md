# Profex — Open Source XRD & Rietveld Refinement

[![License: GPL v2+](https://img.shields.io/badge/License-GPL%20v2%2B-blue.svg)](LICENSE)

**Profex** is a graphical program for Rietveld refinement of powder X-ray diffraction (XRD) data, based on the [BGMN](http://www.bgmn.de/) refinement kernel. It supports phase identification, phase quantification, structure refinement, and provides a large set of convenience features for powder diffraction analysis.

This repository adds **AI-agent-friendly features, MCP (Model Context Protocol) integration, and a standalone Python Search-Match engine** with an auto-generated mineral fingerprint database (2980 entries).

| Attribute        | Value                                      |
|------------------|--------------------------------------------|
| **Version**      | 5.6.1 (Profex) + v0.4.0 (pyprofex)        |
| **Upstream**     | [profex-xrd.org](https://www.profex-xrd.org/) |
| **Kernel**       | BGMN ([bgmn.de](http://www.bgmn.de/))      |
| **License**      | GNU General Public License v2 or later     |
| **Language**     | C++ (Qt 5/6) + Python (Search-Match engine) |
| **Platforms**    | Windows, Linux, macOS                      |

---

## Quick Start — Search-Match CLI

```
# Download the binary from Releases
wget https://github.com/PhaseAnalysisXRD/profex/releases/download/v0.4.0/pyprofex
chmod +x pyprofex

# Identify phases from d-spacings
./pyprofex search 4.255 3.343 2.457 2.282 2.237 --elements Si,O

# Show database statistics
./pyprofex db-info

# List fingerprints in the database
./pyprofex list-db --search quartz

# Run MCP server for AI assistants
./pyprofex mcp
```

The binary is a **25MB standalone executable** containing the full 2980-entry fingerprint database.

---

## pyprofex — Python Search-Match Engine

A Python CLI tool for XRD phase identification, located in `pyprofex/`. Requires no Qt, no compiled binaries — pure Python.

### Commands

| Command | Description |
|---------|-------------|
| `search <d-spacings>` | Identify phases from observed d-spacings |
| `list-db [--search name]` | List/browse the fingerprint database |
| `db-info` | Show database statistics |
| `suggest-elements <d-spacings>` | Suggest elements from observed peak positions |
| `mcp` | Run MCP server for AI assistant integration |

### Fingerprint Database (2980 entries)

| Source | Count | Description |
|--------|-------|-------------|
| **Auto-generated from COD CIF** | 2262 | Calculated with symmetry expansion + structure factor |
| **POWCOD inorganic minerals** | 691 | From Qualx2 database (<150 peaks, mineral names only) |
| **Manually verified** | 27 | Calibrated against known standard patterns |
| **Total** | **2980** | Coverage: 87% of 177 common minerals |

### Performance

- **Load time**: 0.023s (pre-compiled pickle)
- **Single-phase accuracy**: 100%
- **Multi-phase accuracy**: ~30-40% (on 2980-entry database with diverse candidates)
- **Search speed**: <0.5s per sample

---

## Project Structure

```
profex/
├── profex/               # Main GUI application (Qt Widgets)
├── libXrdIO/             # Core I/O and data processing library
│   ├── crystal/          # Crystal structure calculations
│   ├── curveFitting/     # Peak fitting models
│   ├── parser/           # BGMN/CIF/COD file parsers
│   ├── import/           # XRD data format importers (30+ formats)
│   └── export/           # Data export handlers
├── cmdtools/             # Command-line tools
│   ├── pxanytoxy/        # XRD format converter
│   └── pxapplypreset/    # Preset application CLI
├── pyprofex/             # Python Search-Match engine [🚀 NEW]
│   ├── profex_cli.py     # CLI entry point
│   ├── search_match.py   # Unified search engine (manual + auto + POWCOD)
│   ├── cif2fingerprint.py # CIF → powder diffraction calculator
│   ├── mcp_server.py     # MCP server for AI integration
│   ├── peaks.py          # Peak finding and Gaussian fitting
│   ├── fingerprints.py   # Manually verified fingerprints (27)
│   ├── fingerprints_auto.py  # Auto-generated CIF fingerprints (2262)
│   ├── build_fingerprints.py # Pipeline: COD db3 → CIF → fingerprints
│   ├── build_powcod_fingerprints.py  # Qualx2 POWCOD extractor
│   └── build_unified_db.py  # Multi-source fingerprint merger
├── modules/              # Optional standalone modules
├── quazip/               # Zip archive support (bundled)
└── zlib/                 # Compression library (bundled)
```

---

## Build from Source (Qt/C++ Profex)

### Prerequisites

- **Qt 5.12+** or **Qt 6.x** (with widgets, network, xml, concurrent modules)
- **C++17** compatible compiler (GCC 9+, Clang 12+, MSVC 2019+)
- **zlib** development headers

### Quick Build (qmake)

```bash
cd profex
qmake profex.pro
make -j$(nproc)
./profex/profex
```

**On Debian/Ubuntu:**
```bash
sudo apt install build-essential qtbase5-dev qt5-qmake \
  libqt5widgets5 libqt5network5 qttools5-dev-tools zlib1g-dev
```

---

## CIF → Powder Diffraction Engine

The `cif2fingerprint.py` module computes theoretical powder diffraction patterns from CIF files:

1. Parse CIF: cell parameters, space group symmetry operations, atom sites
2. Generate all hkl combinations (d_min = 0.8Å)
3. Calculate d-spacings via reciprocal metric tensor
4. Apply full symmetry expansion to all atom positions
5. Calculate structure factor F(hkl) with Cromer-Mann scattering factors
6. Apply Lorentz-polarization correction
7. Normalize intensities to I_max = 100

**Supported:** 50 common space groups via `_SG_SYMOPS` database (including P3₁21, P6₃mc, R-3c, Fm-3m, etc.), automatic R-3c hexagonal setting detection.

---

## MCP (Model Context Protocol) Support

The MCP server enables AI assistants to perform XRD analysis through structured tool calls:

### Tools

| Tool | Description |
|------|-------------|
| `list_projects` | List Profex project files |
| `convert_file` | Convert XRD data between formats |
| `parse_parameters` | Parse BGMN `.par` files |
| `identify_phases` | **Search-Match**: identify phases from d-spacings (with element hints) |
| `cod_search` | Search Crystallography Open Database |
| `search_phase` | Search available structure files |

### Usage with Claude Desktop / MCP Clients

```json
{
  "mcpServers": {
    "profex": {
      "command": "/path/to/pyprofex",
      "args": ["mcp"]
    }
  }
}
```

---

## Release Binary

Download the standalone 25MB executable from the [Releases page](https://github.com/PhaseAnalysisXRD/profex/releases).

```bash
# Linux x86_64
wget https://github.com/PhaseAnalysisXRD/profex/releases/download/v0.4.0/pyprofex
chmod +x pyprofex
./pyprofex --help
```

> **Note**: Windows and macOS builds are not yet available. The binary can be cross-compiled or run under WSL.

---

## AI Roadmap

### ✅ Completed

- [x] CIF symmetry expansion engine (50 space groups)
- [x] Auto-generated fingerprint database from COD (2262 minerals)
- [x] POWCOD inorganic mineral database (691 entries)
- [x] Multi-source unified fingerprint DB (2980 entries)
- [x] Iterative Search-Match with exclusive scoring
- [x] MCP server with identify_phases tool
- [x] Standalone CLI binary (25MB)
- [x] GitHub Actions CI
- [x] Build scripts (build.sh, setup-dev.sh)
- [x] AI agent config (.clangd, .mcp.json)

### 🔄 Future

- [ ] Cross-platform binary builds (Windows, macOS)
- [ ] Improved multi-phase accuracy via machine learning
- [ ] March-Dollase preferred orientation correction
- [ ] Split Pearson VII peak fitting
- [ ] FOM confidence scoring with uncertainty estimation
- [ ] Direct XRD data file → Search-Match pipeline

---

## Contributing

This is a fork aimed at AI-enhanced XRD analysis. Contributions, issues, and suggestions are welcome.

---

## License

Profex is **free software** released under the **GNU General Public License v2 or later**. See [LICENSE](LICENSE) for details.

BGMN refinement kernel is bundled with permission. Visit [http://www.bgmn.de/](http://www.bgmn.de/) for BGMN source code.

---

## References

- **Profex Website**: [https://www.profex-xrd.org/](https://www.profex-xrd.org/)
- **BGMN Kernel**: [http://www.bgmn.de/](http://www.bgmn.de/)
- **Crystallography Open Database**: [https://www.crystallography.net/](https://www.crystallography.net/)
- **Qualx2**: [https://www.unige.ch/qualx2/](https://www.unige.ch/qualx2/)
