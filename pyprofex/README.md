# pyprofex — Python tools for Profex XRD Analysis

Python package providing:
- XRD calculation utilities (d-spacing, 2-theta, wavelength conversion)
- BGMN parameter file parsing
- Profex CLI wrapper (pxanytoxy, pxapplypreset)
- Data classes for scans and refinement results

## Install

```bash
cd pyprofex
pip install -e .
```

For MCP Server support:
```bash
pip install -e ".[mcp]"
```

## Development

```bash
pip install -e ".[dev]"
pytest
```
