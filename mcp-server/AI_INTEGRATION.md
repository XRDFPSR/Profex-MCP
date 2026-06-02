# Profex MCP Server — AI Editor Integration Guide

The Profex MCP Server provides AI assistants with direct access to XRD data
analysis tools through the [Model Context Protocol (MCP)](https://modelcontextprotocol.io/).

---

## Quick Start

```bash
# Install dependencies
pip install "mcp>=1.0"

# Run the MCP server (stdio mode — for AI editors)
cd profex
python pyprofex/mcp_server.py
```

---

## Integration with AI Code Editors

### VS Code (GitHub Copilot / Cline / Continue)

Add to your VS Code settings (`.vscode/mcp.json`):

```json
{
  "servers": {
    "profex": {
      "type": "stdio",
      "command": "python",
      "args": ["/path/to/profex/pyprofex/mcp_server.py"]
    }
  }
}
```

### Claude Code / Claude Desktop

Add to your `claude_desktop_config.json`:

```json
{
  "mcpServers": {
    "profex": {
      "command": "python",
      "args": ["/path/to/profex/pyprofex/mcp_server.py"]
    }
  }
}
```

### Cursor

Add to Cursor settings → Features → MCP Servers:

```json
{
  "mcpServers": {
    "profex": {
      "command": "python",
      "args": ["/path/to/profex/pyprofex/mcp_server.py"]
    }
  }
}
```

---

## Available Tools

| Tool | Description |
|------|-------------|
| `list_projects` | List available Profex project files |
| `convert_file` | Convert XRD data file to XY format |
| `parse_parameters` | Parse BGMN .par parameter file |
| `list_scans` | List XRD scans in a directory |
| `available_formats` | Show supported import/export formats |
| `search_phase` | Search for phases by name in structure files |

## Available Resources

| Resource URI | Description |
|---|---|
| `profex://project/{name}` | Project metadata |
| `profex://project/{name}/params/{file}` | BGMN refinement parameters |
| `profex://info/formats` | Supported formats reference |

## Available Prompts

| Prompt | Description |
|---|---|
| `analyze_xrd` | Guided XRD analysis workflow with step-by-step instructions |

---

## HTTP / SSE Transport (Remote Access)

```bash
python pyprofex/mcp_server.py --transport sse --port 8100
```

Then connect AI editors to `http://localhost:8100/sse`.

---

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `PROFEX_HOME` | Parent of `pyprofex/` | Profex installation directory |
