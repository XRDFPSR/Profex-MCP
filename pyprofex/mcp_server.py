"""
Profex MCP Server — Model Context Protocol server for XRD analysis.

Provides AI agents with tools and resources to interact with Profex/BGMN
for Rietveld refinement, phase identification, and XRD data analysis.

Usage:
    # Run as stdio server (for AI editors like Claude Code, Cursor, etc.)
    python mcp_server.py

    # Run as HTTP server (for remote access)
    python mcp_server.py --transport sse --port 8100

Requires: pyprofex, mcp>=1.0
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any

import mcp.types as types
from mcp.server import Server, NotificationOptions
from mcp.server.models import InitializationOptions

# ─── Configuration ─────────────────────────────────────────────────────

PROFEX_HOME = Path(os.environ.get("PROFEX_HOME", str(Path(__file__).resolve().parent.parent)))
PROJECTS_DIR = PROFEX_HOME / "projects"
PROJECTS_DIR.mkdir(exist_ok=True)

server = Server("profex")


# ─── Utility Functions ─────────────────────────────────────────────────

def find_profex_bin() -> Path | None:
    """Locate the Profex executable."""
    candidates = [
        PROFEX_HOME / "profex" / "profex",
        PROFEX_HOME / "profex",
        Path("/usr/local/bin/profex"),
    ]
    for c in candidates:
        if c.exists():
            return c
    return None


def find_cmdtool(name: str) -> Path | None:
    """Locate a Profex command-line tool."""
    candidates = [
        PROFEX_HOME / "cmdtools" / name / name,
        PROFEX_HOME / "cmdtools" / name,
    ]
    for c in candidates:
        if c.exists():
            return c
    return None


def list_project_files() -> list[dict]:
    """List all Profex project files in the projects directory."""
    projects = []
    for f in PROJECTS_DIR.glob("*.pro"):
        stat = f.stat()
        projects.append({
            "name": f.stem,
            "path": str(f),
            "size": stat.st_size,
            "modified": stat.st_mtime,
        })
    return sorted(projects, key=lambda p: p["name"])


def parse_bgmn_par(filepath: str | Path) -> dict[str, dict]:
    """Parse a BGMN .par file into a structured dict."""
    params: dict[str, dict] = {}
    try:
        with open(filepath) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or line.startswith("!"):
                    continue
                m = re.match(
                    r'(\w[\w\d_\[\]]*)'  # key, may include [N] for phase index
                    r'\s*=\s*'
                    r'([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)'  # value
                    r'(?:\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?))?',  # optional esd
                    line,
                )
                if m:
                    key = m.group(1)
                    val = float(m.group(2))
                    esd = float(m.group(3)) if m.group(3) else None
                    params[key] = {"value": val, "esd": esd}
    except FileNotFoundError:
        pass
    return params


def scan_to_dict(scan_obj: Any) -> dict:
    """Convert a Scan object (or dict-like) to a serializable dict."""
    if hasattr(scan_obj, "to_dict"):
        return scan_obj.to_dict()
    if isinstance(scan_obj, dict):
        return scan_obj
    return {"name": str(scan_obj)}


# ─── MCP Tools ─────────────────────────────────────────────────────────

@server.list_tools()
async def list_tools() -> list[types.Tool]:
    """Register all available MCP tools."""
    return [
        types.Tool(
            name="list_projects",
            description="List all available Profex project files",
            inputSchema={
                "type": "object",
                "properties": {},
            },
        ),
        types.Tool(
            name="convert_file",
            description="Convert an XRD data file to XY format",
            inputSchema={
                "type": "object",
                "properties": {
                    "input_file": {
                        "type": "string",
                        "description": "Path to input XRD file",
                    },
                    "output_file": {
                        "type": "string",
                        "description": "Path to output .xy file (optional)",
                    },
                },
                "required": ["input_file"],
            },
        ),
        types.Tool(
            name="parse_parameters",
            description="Parse a BGMN .par parameter file into structured data",
            inputSchema={
                "type": "object",
                "properties": {
                    "file": {
                        "type": "string",
                        "description": "Path to BGMN .par file",
                    },
                },
                "required": ["file"],
            },
        ),
        types.Tool(
            name="list_scans",
            description="List XRD scans available in a Profex project directory",
            inputSchema={
                "type": "object",
                "properties": {
                    "directory": {
                        "type": "string",
                        "description": "Directory containing XRD data files",
                    },
                },
                "required": ["directory"],
            },
        ),
        types.Tool(
            name="available_formats",
            description="List supported import and export formats",
            inputSchema={
                "type": "object",
                "properties": {},
            },
        ),
        types.Tool(
            name="search_phase",
            description="Search for a phase by name in the available structure files",
            inputSchema={
                "type": "object",
                "properties": {
                    "query": {
                        "type": "string",
                        "description": "Phase name or chemical element to search",
                    },
                    "directory": {
                        "type": "string",
                        "description": "Directory containing .str structure files (optional)",
                    },
                },
                "required": ["query"],
            },
        ),
    ]


@server.call_tool()
async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:
    """Execute an MCP tool."""
    try:
        if name == "list_projects":
            projects = list_project_files()
            if not projects:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"status": "empty", "projects_dir": str(PROJECTS_DIR)}, indent=2),
                )]
            return [types.TextContent(
                type="text",
                text=json.dumps({"projects": projects}, indent=2),
            )]

        elif name == "convert_file":
            input_file = arguments["input_file"]
            output_file = arguments.get("output_file", str(Path(input_file).with_suffix(".xy")))
            pxanytoxy = find_cmdtool("pxanytoxy")
            if not pxanytoxy:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({
                        "error": "pxanytoxy CLI tool not found. Build it from cmdtools/ first.",
                    }, indent=2),
                )]

            result = subprocess.run(
                [str(pxanytoxy), "-i", input_file, "-o", output_file],
                capture_output=True, text=True, timeout=120,
            )
            if result.returncode != 0:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"error": result.stderr.strip()}, indent=2),
                )]

            return [types.TextContent(
                type="text",
                text=json.dumps({
                    "status": "ok",
                    "input": input_file,
                    "output": output_file,
                }, indent=2),
            )]

        elif name == "parse_parameters":
            filepath = arguments["file"]
            params = parse_bgmn_par(filepath)
            if not params:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"status": "empty", "file": filepath}, indent=2),
                )]
            return [types.TextContent(
                type="text",
                text=json.dumps({"file": filepath, "parameters": params}, indent=2),
            )]

        elif name == "list_scans":
            directory = arguments["directory"]
            dir_path = Path(directory)
            if not dir_path.exists():
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"error": f"Directory not found: {directory}"}, indent=2),
                )]

            extensions = {".xy", ".xrdml", ".raw", ".dat", ".chi", ".xye",
                         ".csv", ".txt", ".ras", ".rd", ".udf", ".brml"}
            scans = []
            for ext in extensions:
                for f in dir_path.glob(f"*{ext}"):
                    stat = f.stat()
                    scans.append({
                        "name": f.name,
                        "path": str(f),
                        "size": stat.st_size,
                        "ext": ext,
                    })

            return [types.TextContent(
                type="text",
                text=json.dumps({
                    "directory": directory,
                    "total_scans": len(scans),
                    "scans": sorted(scans, key=lambda s: s["name"]),
                }, indent=2),
            )]

        elif name == "available_formats":
            data = {
                "import_formats": [
                    "Bruker RAW (V3, V4)", "Bruker BRML", "PANalytical XRDML",
                    "Rigaku RAW/DAT/DIF/RAS/RASX/XML/BIN",
                    "Philips RD/UDF", "Stoe RAW/PRO", "Thermo RAW/NI/TXL",
                    "GSAS Std", "FullProf DAT/PRF/SUB",
                    "Jade MDI/XML", "CHI", "XYE", "XY",
                    "NeXus RAW", "SEIFERT VAL", "pyFAI DAT", "BGMN DIA",
                ],
                "export_formats": [
                    "ASCII XY/TXT/HKL", "GSAS Std", "FullProf DAT",
                    "Fityk FIT", "Gnuplot", "Grace", "Philips UDF", "PDF CIF",
                ],
            }
            return [types.TextContent(
                type="text",
                text=json.dumps(data, indent=2),
            )]

        elif name == "search_phase":
            query = arguments["query"].lower()
            directory = arguments.get("directory", str(PROFEX_HOME / "structures"))
            struct_dir = Path(directory)

            if not struct_dir.exists():
                return [types.TextContent(
                    type="text",
                    text=json.dumps({
                        "error": f"Structure directory not found: {directory}",
                        "hint": "Set PROFEX_HOME or provide a directory argument",
                    }, indent=2),
                )]

            matches = []
            for f in struct_dir.glob("*.str"):
                content = f.read_text(errors="ignore")
                if query in content.lower():
                    # Extract phase name from first few lines
                    first_lines = content.split("\n")[:5]
                    phase_name = f.stem
                    for line in first_lines:
                        m = re.match(r'^\s*\*?\s*([A-Z][a-z]?\w*(?:\s+[A-Z]?[a-z0-9]*)*)', line)
                        if m and not line.startswith("!"):
                            phase_name = m.group(1).strip()
                            break
                    matches.append({
                        "file": f.name,
                        "path": str(f),
                        "phase_name": phase_name,
                        "size": f.stat().st_size,
                    })

            return [types.TextContent(
                type="text",
                text=json.dumps({
                    "query": query,
                    "search_directory": str(struct_dir),
                    "total_matches": len(matches),
                    "matches": matches[:50],
                }, indent=2),
            )]

        else:
            return [types.TextContent(
                type="text",
                text=json.dumps({"error": f"Unknown tool: {name}"}, indent=2),
            )]

    except Exception as e:
        return [types.TextContent(
            type="text",
            text=json.dumps({"error": str(e)}, indent=2),
        )]


# ─── MCP Resources ─────────────────────────────────────────────────────

@server.list_resources()
async def list_resources() -> list[types.Resource]:
    """Register available MCP resources via profex:// protocol."""
    resources = []

    # List projects
    for proj in list_project_files():
        resources.append(
            types.Resource(
                uri=f"profex://project/{proj['name']}",
                name=f"Project: {proj['name']}",
                description=f"Profex project: {proj['name']}",
                mimeType="application/json",
            )
        )
        # Check for .par files
        proj_dir = Path(proj['path']).parent
        for par_file in proj_dir.glob(f"{proj['name']}*.par"):
            par_name = par_file.stem
            resources.append(
                types.Resource(
                    uri=f"profex://project/{proj['name']}/params/{par_name}",
                    name=f"Parameters: {par_name}",
                    description=f"BGMN parameter file for {proj['name']}",
                    mimeType="application/json",
                )
            )

    # Formats info
    resources.append(
        types.Resource(
            uri="profex://info/formats",
            name="Profex supported formats",
            description="List of supported import and export file formats",
            mimeType="application/json",
        )
    )

    return resources


@server.read_resource()
async def read_resource(uri: str) -> str:
    """Read an MCP resource from the profex:// protocol."""
    # profex://info/formats
    if uri == "profex://info/formats":
        data = {
            "import": ["Bruker RAW", "PANalytical XRDML", "Rigaku", "Philips",
                       "Stoe", "Thermo", "GSAS", "FullProf", "Jade", "CHI/XYE/XY"],
            "export": ["ASCII", "GSAS", "FullProf", "Fityk", "Gnuplot", "Grace", "PDF CIF"],
        }
        return json.dumps(data, indent=2)

    # profex://project/{name}[/...]
    m = re.match(r'^profex://project/([^/]+)(?:/params/([^/]+))?$', uri)
    if m:
        proj_name = m.group(1)
        par_name = m.group(2)

        proj_file = PROJECTS_DIR / f"{proj_name}.pro"
        if not proj_file.exists():
            raise ValueError(f"Project '{proj_name}' not found")

        if par_name:
            # Search for .par file matching the name
            for pf in PROJECTS_DIR.glob(f"{par_name}*"):
                params = parse_bgmn_par(pf)
                return json.dumps({
                    "file": str(pf),
                    "parameters": params,
                }, indent=2)
            raise ValueError(f"Parameter file '{par_name}' not found")

        return json.dumps({
            "name": proj_name,
            "file": str(proj_file),
            "exists": True,
        }, indent=2)

    raise ValueError(f"Unknown resource: {uri}")


# ─── Prompts ───────────────────────────────────────────────────────────

@server.list_prompts()
async def list_prompts() -> list[types.Prompt]:
    return [
        types.Prompt(
            name="analyze_xrd",
            description="Analyze an XRD dataset with Profex/BGMN",
            arguments=[
                types.PromptArgument(
                    name="data_file",
                    description="Path to XRD data file",
                    required=True,
                ),
                types.PromptArgument(
                    name="project_name",
                    description="Name for the new Profex project",
                    required=False,
                ),
            ],
        ),
    ]


@server.get_prompt()
async def get_prompt(
    name: str, arguments: dict[str, str] | None
) -> types.GetPromptResult:
    if name == "analyze_xrd":
        data_file = (arguments or {}).get("data_file", "unknown")
        return types.GetPromptResult(
            description="XRD Analysis Workflow",
            messages=[
                types.PromptMessage(
                    role="user",
                    content=types.TextContent(
                        type="text",
                        text=(
                            "I want to analyze the XRD data file at: "
                            f"{data_file}\n\n"
                            "Please help me:\n"
                            "1. List the available formats with the "
                            "available_formats tool\n"
                            "2. Convert the file to XY format\n"
                            "3. If this is a Rietveld refinement project, "
                            "set up the refinement parameters\n"
                            "4. Parse the results and provide a summary"
                        ),
                    ),
                ),
            ],
        )

    raise ValueError(f"Unknown prompt: {name}")


# ─── Server Entry Point ───────────────────────────────────────────────

async def main() -> None:
    """Run the MCP server."""
    import argparse

    parser = argparse.ArgumentParser(description="Profex MCP Server")
    parser.add_argument(
        "--transport",
        choices=["stdio", "sse"],
        default="stdio",
        help="Transport protocol (default: stdio for AI editor integration)",
    )
    parser.add_argument("--port", type=int, default=8100, help="Port for SSE transport")
    parser.add_argument("--host", type=str, default="0.0.0.0", help="Host for SSE transport")
    args = parser.parse_args()

    if args.transport == "sse":
        from mcp.server.sse import SseServerTransport
        from starlette.applications import Starlette
        from starlette.routing import Mount, Route

        sse = SseServerTransport("/messages/")

        async def handle_sse(request):
            async with sse.connect_sse(
                request.scope, request.receive, request._send,
            ) as streams:
                await server.run(
                    streams[0], streams[1],
                    InitializationOptions(
                        server_name="profex",
                        server_version="0.1.0",
                    ),
                )

        app = Starlette(
            routes=[
                Route("/sse", endpoint=handle_sse),
                Mount("/messages/", app=sse.handle_post_message),
            ],
        )

        import uvicorn
        print(f"Profex MCP Server running on http://{args.host}:{args.port}/sse", file=sys.stderr)
        uvicorn.run(app, host=args.host, port=args.port)
    else:
        # stdio transport — default for AI code editor integration
        async with server.run_stdio() as streams:
            await server.run(
                streams[0], streams[1],
                InitializationOptions(
                    server_name="profex",
                    server_version="0.1.0",
                ),
            )


if __name__ == "__main__":
    import asyncio
    asyncio.run(main())
