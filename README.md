# Profex — Open Source XRD & Rietveld Refinement

[![License: GPL v2+](https://img.shields.io/badge/License-GPL%20v2%2B-blue.svg)](LICENSE)

**Profex** is a graphical program for Rietveld refinement of powder X-ray diffraction (XRD) data, based on the [BGMN](http://www.bgmn.de/) refinement kernel. It supports phase identification, phase quantification, structure refinement, and provides a large set of convenience features for powder diffraction analysis.

This repository is a fork of the upstream Profex source code, maintained with the goal of making the codebase **AI-agent-friendly** and adding **MCP (Model Context Protocol) support** for seamless integration with AI-assisted development and analysis workflows.

| Attribute        | Value                                      |
|------------------|--------------------------------------------|
| **Version**      | 5.6.1                                      |
| **Upstream**     | [profex-xrd.org](https://www.profex-xrd.org/) |
| **Kernel**       | BGMN ([bgmn.de](http://www.bgmn.de/))      |
| **License**      | GNU General Public License v2 or later     |
| **Language**     | C++ (Qt 5/6)                               |
| **Platforms**    | Windows, Linux, macOS                      |

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
├── modules/              # Optional standalone modules
│   ├── electrondensity/  # Electron density map viewer
│   ├── scantracer/       # Scan digitization from images
│   ├── synchrotronconfigurator/  # Synchrotron beamline config
│   └── waterfall/        # Waterfall plot viewer
├── quazip/               # Zip archive support (bundled)
└── zlib/                 # Compression library (bundled)
```

---

## Build from Source

### Prerequisites

- **Qt 5.12+** or **Qt 6.x** (with widgets, network, xml, concurrent modules)
- **C++17** compatible compiler (GCC 9+, Clang 12+, MSVC 2019+)
- **CMake 3.16+** (optional, for quazip) or qmake
- **zlib** development headers

### Quick Build (qmake)

```bash
# Ensure Qt binaries are in PATH
export PATH=/path/to/Qt/version/gcc_64/bin:$PATH

# Build
cd profex
qmake profex.pro
make -j$(nproc)

# Run
./profex/profex
```

### Quick Build (CMake — experimental)

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build . -j$(nproc)
```

### Build Requirements by Platform

**On Debian/Ubuntu:**
```bash
sudo apt install build-essential qtbase5-dev qt5-qmake \
  libqt5widgets5 libqt5network5 qttools5-dev-tools zlib1g-dev
```

**On Fedora:**
```bash
sudo dnf install qt5-qtbase-devel qt5-qttools-devel zlib-devel
```

**On macOS (Homebrew):**
```bash
brew install qt@5
```

---

## AI-Agent-Friendly Improvements (Roadmap)

This fork aims to make Profex's codebase more accessible to AI coding agents and enable AI-assisted XRD analysis. Planned improvements:

### Phase 1 — Codebase Quality (Current)
- [x] Add LICENSE file
- [x] Write comprehensive README with build instructions
- [ ] Add `.gitignore` for Qt/C++ builds
- [ ] Add `Doxygen` documentation comments to core APIs
- [ ] Clean up compiler warnings with modern C++ standards

### Phase 2 — AI Agent Usability
- [ ] Add structured build scripts (`scripts/build.sh`, `scripts/build.ps1`)
- [ ] Add QML-based CLI interface for headless operation
- [ ] Expose key algorithms via JSON/YAML configuration
- [ ] Add Python bindings (pybind11) for core refinement routines

### Phase 3 — MCP Integration
- [x] Add MCP server configuration
- [ ] Add structured data output (JSON schema for refinement results)
- [ ] Expose BGMN refinement as MCP tools for AI agents
- [ ] Add MCP resources for phase identification results
- [ ] Enable AI-driven refinement parameter optimization

---

## MCP Support

This repository includes [MCP (Model Context Protocol)](https://modelcontextprotocol.io/) configuration to enable AI assistants to interact with the Profex codebase and its analysis results.

- **`.mcp.json`** — MCP server configuration for AI code editors
- **`mcp-server/`** — (Planned) MCP server providing XRD analysis tools

See [MCP documentation](mcp-docs/README.md) for setup instructions.

---

## Contributing

This is a fork intended for AI-enhanced development. Contributions, issues, and suggestions are welcome. Please open an issue or pull request for any improvements.

---

## License

Profex is **free software** released under the **GNU General Public License v2 or later**. See [LICENSE](LICENSE) for details.

The BGMN refinement kernel is bundled with permission of the BGMN development team. Visit [http://www.bgmn.de/](http://www.bgmn.de/) for the BGMN source code and documentation.

---

## References

- **Profex Website**: [https://www.profex-xrd.org/](https://www.profex-xrd.org/)
- **BGMN Kernel**: [http://www.bgmn.de/](http://www.bgmn.de/)
- **Crystallography Open Database (COD)**: [https://www.crystallography.net/](https://www.crystallography.net/)
