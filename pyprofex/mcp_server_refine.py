"""
Profex MCP Server — Extended Tools for Phase 5 AI Agent Integration.

This module provides deeper AI agent capabilities for XRD analysis:
- run_refinement: execute BGMN refinement on .sav control files
- get_results: parse refinement results from .par files
- batch_refine: multi-parameter exploration workflows
- refinement report generation

All tools produce structured JSON output consumable by AI agents.
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import tempfile
import time
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Any

import mcp.types as types
from mcp.server import Server

# ─── Reuse configuration from main server ──────────────────────────────

PROFEX_HOME = Path(os.environ.get("PROFEX_HOME", str(Path(__file__).resolve().parent.parent)))

server_refine = Server("profex-refine")


# ─── Data Models ───────────────────────────────────────────────────────

@dataclass
class PhaseResult:
    name: str
    weight_fraction: float
    weight_fraction_esd: float | None = None
    a: float | None = None
    b: float | None = None
    c: float | None = None
    cell_volume: float | None = None
    r_bragg: float | None = None
    grain_size_nm: float | None = None
    microstrain: float | None = None


@dataclass
class RefinementSummary:
    project_name: str
    control_file: str
    status: str  # completed, failed, running, idle
    rwp: float | None = None
    rexp: float | None = None
    gof: float | None = None
    durbin_watson: float | None = None
    total_cycles: int | None = None
    phases: list[PhaseResult] = None
    bgmn_output: str = ""
    error_log: str = ""

    def to_dict(self) -> dict:
        d = asdict(self)
        d["phases"] = [asdict(p) for p in (self.phases or [])]
        return d


# ─── Utility Functions ─────────────────────────────────────────────────

def find_bgmn_executable() -> Path | None:
    """Locate the BGMN executable."""
    candidates = [
        PROFEX_HOME / "bgmn" / "bgmn",
        PROFEX_HOME / "bgmn" / "bgmn.exe",
        Path("/usr/local/bin/bgmn"),
        Path("/usr/bin/bgmn"),
    ]
    # Also check common subdirectories
    for root in [PROFEX_HOME, PROFEX_HOME / "lib", PROFEX_HOME / "bin"]:
        for pattern in ["bgmn*", "bgmn.exe"]:
            for f in root.glob(pattern):
                candidates.append(f)
    for c in candidates:
        if c.exists() and os.access(str(c), os.X_OK):
            return c
    return None


def parse_bgmn_par(filepath: str | Path) -> dict[str, dict]:
    """Parse a BGMN .par parameter file."""
    params: dict[str, dict] = {}
    try:
        with open(filepath) as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or line.startswith("!"):
                    continue
                m = re.match(
                    r'(\w[\w\d\[\]]*)'  # key, may include [N] for phase index
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


def parse_bgmn_lst(filepath: str | Path) -> RefinementSummary:
    """Parse a BGMN .lst output file to extract R-factors and phase info."""
    summary = RefinementSummary(
        project_name=Path(filepath).stem,
        control_file=str(filepath),
        status="completed",
    )
    try:
        content = Path(filepath).read_text(errors="replace")

        # Extract Rwp / Rexp / GoF
        for line in content.split("\n"):
            m = re.search(r'Rwp\s*[=:]\s*([\d.]+)', line, re.IGNORECASE)
            if m and summary.rwp is None:
                summary.rwp = float(m.group(1))
            m = re.search(r'Rexp\s*[=:]\s*([\d.]+)', line, re.IGNORECASE)
            if m and summary.rexp is None:
                summary.rexp = float(m.group(1))
            m = re.search(r'GoF\s*[=:]\s*([\d.]+)', line, re.IGNORECASE)
            if m and summary.gof is None:
                summary.gof = float(m.group(1))
            m = re.search(r'DW\s*[=:]\s*([\d.]+)', line, re.IGNORECASE)
            if m and summary.durbin_watson is None:
                summary.durbin_watson = float(m.group(1))

        # Count cycles
        summary.total_cycles = content.count("Cycle")
    except FileNotFoundError:
        summary.status = "failed"
        summary.error_log = "File not found"

    return summary


def parse_bgmn_output(text: str) -> dict:
    """Parse raw BGMN console output for structured information."""
    result = {
        "cycles": [],
        "warnings": [],
        "errors": [],
        "completion": False,
    }
    for line in text.split("\n"):
        m = re.search(r'Cycle\s+(\d+)', line, re.IGNORECASE)
        if m:
            result["cycles"].append(int(m.group(1)))
        if "error" in line.lower():
            result["errors"].append(line.strip())
        if "warning" in line.lower():
            result["warnings"].append(line.strip())
        if "completed" in line.lower() or "finished" in line.lower():
            result["completion"] = True
    return result


def find_sav_files(directory: str | Path) -> list[dict]:
    """Find all BGMN .sav control files in a directory."""
    dir_path = Path(directory)
    if not dir_path.exists():
        return []
    files = []
    for f in dir_path.glob("*.sav"):
        stat = f.stat()
        # Quick check: read first few bytes to verify it's a BGMN control file
        try:
            header = f.read_bytes()[:100]
            is_bgmn = b"VERZERR" in header or b"LAMBDA" in header or b"STRUC" in header
        except Exception:
            is_bgmn = False
        files.append({
            "name": f.name,
            "path": str(f),
            "size": stat.st_size,
            "modified": stat.st_mtime,
            "is_bgmn_control": is_bgmn,
        })
    return sorted(files, key=lambda x: x["name"])


# ─── Phase Extraction from .par files ──────────────────────────────────

def extract_phases_from_par(params: dict[str, dict]) -> list[PhaseResult]:
    """Extract phase information from parsed BGMN .par parameters."""
    phases = []

    # Group parameters by phase index (e.g. SCALE[1], A[1], SCALE[2], A[2]...)
    phase_indices: set[int] = set()
    for key in params:
        m = re.match(r'(\w+)\[(\d+)\]', key)
        if m:
            phase_indices.add(int(m.group(2)))

    for idx in sorted(phase_indices):
        phase = PhaseResult(
            name=f"Phase {idx}",
            weight_fraction=0.0,
        )

        # Extract parameters for this phase
        wf_key = f"WF[{idx}]"
        wf_esd_key = f"WF_ESD[{idx}]"
        scale_key = f"SCALE[{idx}]"

        if wf_key in params:
            phase.weight_fraction = params[wf_key]["value"]
            if wf_esd_key in params:
                phase.weight_fraction_esd = params[wf_esd_key]["value"]
        elif scale_key in params:
            # SCALE can be converted to approximate WF
            phase.weight_fraction = params[scale_key]["value"]

        for cell_param in ["A", "B", "C"]:
            key = f"{cell_param}[{idx}]"
            if key in params:
                setattr(phase, cell_param.lower(), params[key]["value"])

        vol_key = f"VOL[{idx}]"
        if vol_key in params:
            phase.cell_volume = params[vol_key]["value"]

        rb_key = f"R_BRAGG[{idx}]"
        if rb_key in params:
            phase.r_bragg = params[rb_key]["value"]

        gs_key = f"GRAIN_SIZE[{idx}]"
        if gs_key in params:
            phase.grain_size_nm = params[gs_key]["value"]

        phases.append(phase)

    # If no bracketed params, try single-phase format
    if not phases:
        phase = PhaseResult(name="Default Phase", weight_fraction=100.0)
        for cp in ["a", "b", "c"]:
            if cp in params:
                setattr(phase, cp, params[cp]["value"])
        vol = params.get("VOL")
        if vol:
            phase.cell_volume = vol["value"]
        phases.append(phase)

    return phases


# ─── MCP Tools ─────────────────────────────────────────────────────────

@server_refine.list_tools()
async def list_tools() -> list[types.Tool]:
    """Register Phase 5 extended tools."""
    return [
        types.Tool(
            name="run_refinement",
            description="Execute a BGMN refinement using a .sav control file",
            inputSchema={
                "type": "object",
                "properties": {
                    "sav_file": {
                        "type": "string",
                        "description": "Path to BGMN .sav control file",
                    },
                    "working_dir": {
                        "type": "string",
                        "description": "Working directory for refinement (default: .sav file directory)",
                    },
                    "timeout_seconds": {
                        "type": "integer",
                        "description": "Max refinement time in seconds",
                        "default": 300,
                    },
                    "extra_args": {
                        "type": "string",
                        "description": "Additional BGMN command-line arguments",
                    },
                },
                "required": ["sav_file"],
            },
        ),
        types.Tool(
            name="get_results",
            description="Extract refinement results from .par and .lst output files",
            inputSchema={
                "type": "object",
                "properties": {
                    "directory": {
                        "type": "string",
                        "description": "Directory containing .par and .lst files",
                    },
                    "project_name": {
                        "type": "string",
                        "description": "Filter by project name (optional)",
                    },
                },
                "required": ["directory"],
            },
        ),
        types.Tool(
            name="batch_refine",
            description="Run multiple refinements with parameter variations for AI-driven optimization",
            inputSchema={
                "type": "object",
                "properties": {
                    "base_sav_file": {
                        "type": "string",
                        "description": "Base .sav control file",
                    },
                    "parameter_variations": {
                        "type": "string",
                        "description": "JSON object with parameter name -> list of values to try",
                    },
                    "timeout_per_run": {
                        "type": "integer",
                        "description": "Timeout per refinement run in seconds",
                        "default": 120,
                    },
                },
                "required": ["base_sav_file", "parameter_variations"],
            },
        ),
        types.Tool(
            name="list_sav_files",
            description="List available BGMN .sav control files in a directory",
            inputSchema={
                "type": "object",
                "properties": {
                    "directory": {
                        "type": "string",
                        "description": "Directory to scan for .sav files",
                    },
                },
                "required": ["directory"],
            },
        ),
    ]


@server_refine.call_tool()
async def call_tool(name: str, arguments: dict) -> list[types.TextContent]:
    """Execute an extended Phase 5 MCP tool."""
    try:
        if name == "run_refinement":
            sav_file = arguments["sav_file"]
            working_dir = arguments.get("working_dir", str(Path(sav_file).parent))
            timeout = arguments.get("timeout_seconds", 300)
            extra_args = arguments.get("extra_args", "")

            bgmn = find_bgmn_executable()
            if not bgmn:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({
                        "error": "BGMN executable not found. Ensure bgmn is available in PROFEX_HOME.",
                        "hint": "BGMN is distributed separately from Profex. See https://www.profex-xrd.org/",
                    }, indent=2),
                )]

            # Validate .sav file
            sav_path = Path(sav_file)
            if not sav_path.exists():
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"error": f".sav file not found: {sav_file}"}, indent=2),
                )]

            # Run BGMN
            cmd = [str(bgmn), str(sav_path)]
            if extra_args:
                cmd.extend(extra_args.split())

            try:
                result = subprocess.run(
                    cmd,
                    capture_output=True,
                    text=True,
                    timeout=timeout,
                    cwd=working_dir,
                )
            except subprocess.TimeoutExpired:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({
                        "status": "timeout",
                        "sav_file": sav_file,
                        "timeout_seconds": timeout,
                    }, indent=2),
                )]

            # Parse output
            stdout_parsed = parse_bgmn_output(result.stdout)
            stderr_parsed = parse_bgmn_output(result.stderr) if result.stderr else {}

            # Build summary
            summary = RefinementSummary(
                project_name=sav_path.stem,
                control_file=str(sav_path),
                status="completed" if result.returncode == 0 else "failed",
                bgmn_output=result.stdout[-2000:] if len(result.stdout) > 2000 else result.stdout,
                error_log=result.stderr[-1000:] if result.stderr else "",
            )

            # Try to read .par and .lst files that were generated
            dir_path = Path(working_dir)
            base = sav_path.stem
            for par_file in dir_path.glob(f"{base}*.par"):
                params = parse_bgmn_par(par_file)
                summary.phases = extract_phases_from_par(params)
                break

            for lst_file in dir_path.glob(f"{base}*.lst"):
                lst_summary = parse_bgmn_lst(lst_file)
                summary.rwp = lst_summary.rwp or summary.rwp
                summary.rexp = lst_summary.rexp
                summary.gof = lst_summary.gof
                summary.durbin_watson = lst_summary.durbin_watson
                summary.total_cycles = lst_summary.total_cycles
                break

            return [types.TextContent(
                type="text",
                text=json.dumps(summary.to_dict(), indent=2),
            )]

        elif name == "get_results":
            directory = arguments["directory"]
            project_name = arguments.get("project_name", "")
            dir_path = Path(directory)

            if not dir_path.exists():
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"error": f"Directory not found: {directory}"}, indent=2),
                )]

            # Find all .par and .lst files
            results = []
            pattern = f"{project_name}*" if project_name else "*"
            for par_file in sorted(dir_path.glob(f"{pattern}.par")):
                params = parse_bgmn_par(par_file)
                phases = extract_phases_from_par(params)
                summary = RefinementSummary(
                    project_name=par_file.stem,
                    control_file=str(par_file),
                    status="completed",
                    phases=phases,
                )

                # Look for corresponding .lst
                lst_file = par_file.with_suffix(".lst")
                if lst_file.exists():
                    lst_summary = parse_bgmn_lst(lst_file)
                    summary.rwp = lst_summary.rwp
                    summary.rexp = lst_summary.rexp
                    summary.gof = lst_summary.gof
                    summary.durbin_watson = lst_summary.durbin_watson
                    summary.total_cycles = lst_summary.total_cycles

                results.append(summary.to_dict())

            if not results:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({
                        "status": "no_results",
                        "directory": directory,
                        "hint": "No .par or .lst files found. Run a refinement first.",
                    }, indent=2),
                )]

            return [types.TextContent(
                type="text",
                text=json.dumps({"results": results, "total": len(results)}, indent=2),
            )]

        elif name == "batch_refine":
            base_sav = arguments["base_sav_file"]
            param_variations = json.loads(arguments["parameter_variations"])
            timeout = arguments.get("timeout_per_run", 120)

            bgmn = find_bgmn_executable()
            if not bgmn:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"error": "BGMN executable not found"}, indent=2),
                )]

            sav_path = Path(base_sav)
            if not sav_path.exists():
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"error": f".sav file not found: {base_sav}"}, indent=2),
                )]

            # Generate all parameter combinations
            param_names = list(param_variations.keys())
            if not param_names:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({"error": "No parameter variations provided"}, indent=2),
                )]

            import itertools
            values_lists = [param_variations[name] for name in param_names]
            combinations = list(itertools.product(*values_lists))

            if len(combinations) > 50:
                return [types.TextContent(
                    type="text",
                    text=json.dumps({
                        "error": f"Too many combinations ({len(combinations)}). Maximum is 50.",
                        "combinations_requested": len(combinations),
                    }, indent=2),
                )]

            # Run refinements
            output_dir = Path(base_sav).parent
            batch_results = []
            for i, combo in enumerate(combinations):
                combo_name = "_".join(f"{p}={v}" for p, v in zip(param_names, combo))
                run = {
                    "run": i + 1,
                    "parameters": dict(zip(param_names, combo)),
                    "status": "pending",
                }

                # Create modified .sav file (create a temp copy)
                sav_content = sav_path.read_text()
                for pname, pval in zip(param_names, combo):
                    # Replace parameter in .sav file
                    sav_content = re.sub(
                        rf'^{pname}\s*=.*$', f"{pname}={pval}", sav_content, flags=re.MULTILINE,
                    )

                temp_sav = output_dir / f"batch_{sav_path.stem}_{i:03d}.sav"
                temp_sav.write_text(sav_content)

                try:
                    proc = subprocess.run(
                        [str(bgmn), str(temp_sav)],
                        capture_output=True, text=True,
                        timeout=timeout, cwd=str(output_dir),
                    )
                    run["status"] = "completed" if proc.returncode == 0 else "failed"
                    run["output"] = proc.stdout[-500:]

                    # Try to extract Rwp
                    for line in proc.stdout.split("\n"):
                        m = re.search(r'Rwp\s*[=:]\s*([\d.]+)', line, re.IGNORECASE)
                        if m:
                            run["rwp"] = float(m.group(1))
                            break
                except subprocess.TimeoutExpired:
                    run["status"] = "timeout"

                batch_results.append(run)

            best = min(
                [r for r in batch_results if r.get("rwp") is not None],
                key=lambda r: r["rwp"],
                default=None,
            )

            return [types.TextContent(
                type="text",
                text=json.dumps({
                    "total_runs": len(batch_results),
                    "best_result": best,
                    "results": batch_results,
                }, indent=2),
            )]

        elif name == "list_sav_files":
            directory = arguments["directory"]
            files = find_sav_files(directory)
            return [types.TextContent(
                type="text",
                text=json.dumps({"directory": directory, "files": files}, indent=2),
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


# ─── Extended Resources ────────────────────────────────────────────────

@server_refine.list_resources()
async def list_resources() -> list[types.Resource]:
    """Register refinement-specific resources."""
    return [
        types.Resource(
            uri="profex://refinement/example",
            name="Example Refinement",
            description="Example BGMN refinement with LaB6 (NIST standard)",
            mimeType="application/json",
        ),
    ]


@server_refine.read_resource()
async def read_resource(uri: str) -> str:
    """Read refinement resources."""
    if uri == "profex://refinement/example":
        example = {
            "title": "LaB6 NIST Standard Refinement",
            "description": "Example Rietveld refinement of NIST SRM 660c LaB6",
            "files": {
                "control": "oqproject/LaB6_PW1800_5-120.sav",
                "structure": "oqproject/LaB6.str",
                "data": "oqproject/LaB6_PW1800_5-120.xrdml",
                "instrument": "oqproject/pw1800-ads-10mm.geq",
            },
            "expected_results": {
                "a_angstrom": 4.1569,
                "rwp": "~10-15%",
                "phases": "1 (LaB6)",
            },
            "workflow": [
                "1. Convert XRDML to XY: profex-cli convert LaB6.xrdml",
                "2. Run BGMN on .sav file",
                "3. Parse results from .par file",
            ],
        }
        return json.dumps(example, indent=2)

    raise ValueError(f"Unknown resource: {uri}")


# ─── Refinement Advisor Prompt ─────────────────────────────────────────

@server_refine.list_prompts()
async def list_prompts() -> list[types.Prompt]:
    return [
        types.Prompt(
            name="refine_advisor",
            description="AI Refinement Advisor — get guidance on Rietveld refinement parameters",
            arguments=[
                types.PromptArgument(
                    name="material",
                    description="Material/system being refined",
                    required=True,
                ),
                types.PromptArgument(
                    name="rwp",
                    description="Current Rwp value (if available)",
                    required=False,
                ),
                types.PromptArgument(
                    name="issue",
                    description="Description of refinement problem (e.g. high background, peak mismatch)",
                    required=False,
                ),
            ],
        ),
        types.Prompt(
            name="search_match_workflow",
            description="Guided phase identification workflow using COD database",
            arguments=[
                types.PromptArgument(
                    name="elements",
                    description="Expected elements (comma-separated, e.g. Si,O)",
                    required=True,
                ),
                types.PromptArgument(
                    name="peak_positions",
                    description="Observed peak positions in 2-theta (comma-separated)",
                    required=False,
                ),
            ],
        ),
    ]


@server_refine.get_prompt()
async def get_prompt(
    name: str, arguments: dict[str, str] | None
) -> types.GetPromptResult:
    args = arguments or {}

    if name == "refine_advisor":
        material = args.get("material", "unknown")
        rwp = args.get("rwp", "N/A")
        issue = args.get("issue", "general")

        return types.GetPromptResult(
            description="Rietveld Refinement Guidance",
            messages=[
                types.PromptMessage(
                    role="user",
                    content=types.TextContent(
                        type="text",
                        text=(
                            f"I am refining {material} in Profex/BGMN.\n"
                            f"Current Rwp: {rwp}\n"
                            f"Issue: {issue}\n\n"
                            "Please help me:\n"
                            "1. Identify which parameters to refine first\n"
                            "2. Suggest reasonable starting values\n"
                            "3. Interpret current R-factors\n"
                            "4. Diagnose common issues (background, peak shape, zero shift)\n"
                            "5. Suggest next steps to improve the fit"
                        ),
                    ),
                ),
            ],
        )

    if name == "search_match_workflow":
        elements = args.get("elements", "unknown")
        peaks = args.get("peak_positions", "not provided")

        return types.GetPromptResult(
            description="Phase Identification Workflow",
            messages=[
                types.PromptMessage(
                    role="user",
                    content=types.TextContent(
                        type="text",
                        text=(
                            "I need to identify phases in my XRD pattern.\n\n"
                            f"Expected elements: {elements}\n"
                            f"Observed peaks (2-theta): {peaks}\n\n"
                            "Please guide me through:\n"
                            "1. Search the COD database for candidate phases\n"
                            "2. Match observed peaks against known patterns\n"
                            "3. Suggest which phases to include in refinement\n"
                            "4. Provide d-spacings and relative intensities for candidates"
                        ),
                    ),
                ),
            ],
        )

    raise ValueError(f"Unknown prompt: {name}")


# ─── Standalone Entry Point ────────────────────────────────────────────

if __name__ == "__main__":
    import asyncio

    async def main():
        async with server_refine.run_stdio() as streams:
            await server_refine.run(
                streams[0], streams[1],
                server_refine.create_initialization_options(),
            )

    asyncio.run(main())
