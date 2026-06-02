# MCP Server — Profex XRD Analysis

This directory will contain the **MCP (Model Context Protocol) server** for Profex,
enabling AI assistants to directly interact with XRD refinement workflows.

## Planned Tools

The following MCP tools are planned for development:

| Tool Name              | Description                                        |
|------------------------|----------------------------------------------------|
| `run_refinement`       | Execute a BGMN refinement with given parameters    |
| `get_phase_results`    | Retrieve phase quantification results              |
| `list_scans`           | List loaded XRD scans in a project                 |
| `export_results`       | Export refinement results in structured JSON       |
| `search_phase`         | Search phases by name or chemistry                 |
| `optimize_parameters`  | AI-driven parameter optimization for refinement    |

## Planned Resources

| Resource URI Pattern             | Description                            |
|----------------------------------|----------------------------------------|
| `profex://project/{id}/phases`   | Phase composition data                 |
| `profex://project/{id}/scans`    | Loaded diffraction scans               |
| `profex://project/{id}/report`   | HTML refinement report                 |

## Development

The MCP server will be implemented as a standalone Python or Node.js service
that wraps the Profex command-line tools and parses structured output.

```python
# Example structure (future)
from mcp.server import Server
from mcp.types import Tool, Resource

server = Server("profex")

@server.list_tools()
async def list_tools():
    return [
        Tool(
            name="run_refinement",
            description="Execute BGMN refinement",
            inputSchema={...}
        ),
    ]
```

## Usage with AI Editors

Add the following to your AI editor's MCP config:

```json
{
  "mcpServers": {
    "profex": {
      "command": "python",
      "args": ["-m", "mcp_server_profex"],
      "env": {
        "PROFEX_HOME": "/path/to/profex"
      }
    }
  }
}
```
