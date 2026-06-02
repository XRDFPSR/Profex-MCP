#!/usr/bin/env python3
"""
profex-cli — Profex Headless Command-Line Interface

Provides structured access to Profex XRD analysis capabilities without
requiring the Qt GUI. Acts as a bridge between Profex and AI agents.

Usage:
    profex-cli convert input.raw output.xy
    profex-cli refine project.pro [--preset default]
    profex-cli results project.pro --json
    profex-cli parse-params bgmn.par
    profex-cli info supported-formats
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path

# Global JSON flag, set by main() from parsed args
_json_output = False


def json_or_print(data: dict, plain: str) -> None:
    """Print data as JSON or plain text based on global --json flag."""
    global _json_output
    if _json_output:
        print(json.dumps(data, indent=2, default=str))
    else:
        print(plain)


def find_profex_home() -> Path:
    """Locate the Profex installation directory."""
    env_home = os.environ.get("PROFEX_HOME")
    if env_home:
        return Path(env_home)
    # Check relative to this script
    script_dir = Path(__file__).resolve().parent
    for candidate in [script_dir, script_dir.parent, script_dir.parents[1]]:
        if (candidate / "profex" / "profex").exists() or (candidate / "profex.pro").exists():
            return candidate
    return Path.cwd()


def cmd_convert(args: argparse.Namespace) -> None:
    """Convert XRD data file to XY format."""
    profex_home = find_profex_home()
    pxanytoxy = profex_home / "cmdtools" / "pxanytoxy" / "pxanytoxy"

    if not pxanytoxy.exists():
        print(f"Error: pxanytoxy not found at {pxanytoxy}. Build it first.", file=sys.stderr)
        sys.exit(1)

    output = args.output or Path(args.input).with_suffix(".xy")
    cmd = [str(pxanytoxy), "-i", str(args.input), "-o", str(output)]
    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print(f"Error: {result.stderr}", file=sys.stderr)
        sys.exit(1)

    if args.json:
        print(json.dumps({"input": str(args.input), "output": str(output), "status": "ok"}))
    else:
        print(f"Converted: {args.input} -> {output}")


def cmd_info(args: argparse.Namespace) -> None:
    """Display Profex information."""
    if args.topic == "supported-formats":
        info = {
            "import": [
                "Bruker RAW (V3, V4)", "Bruker BRML", "PANalytical XRDML",
                "Rigaku RAW/DAT/DIF/RAS/RASX/XML/BIN",
                "Philips RD/UDF", "Stoe RAW/PRO", "Thermo RAW/NI/TXL",
                "GSAS Std, FullProf DAT/PRF/SUB", "Jade MDI/XML",
                "CHI, XYE, XY", "NeXus RAW", "SEIFERT VAL",
                "pyFAI DAT", "BGMN DIA",
            ],
            "export": [
                "ASCII XY/TXT/HKL", "GSAS Std", "FullProf DAT",
                "Fityk FIT", "Gnuplot", "Grace (xmgrace)",
                "Philips UDF", "PDF CIF",
            ],
            "version": "Profex 5.6.1",
        }
        if args.json:
            print(json.dumps(info, indent=2))
        else:
            print("Supported Import Formats:")
            for fmt in info["import"]:
                print(f"  - {fmt}")
            print("\nSupported Export Formats:")
            for fmt in info["export"]:
                print(f"  - {fmt}")
    else:
        print(f"Unknown topic: {args.topic}")


def cmd_parse_params(args: argparse.Namespace) -> None:
    """Parse a BGMN .par file and output structured data."""
    import re

    params: dict[str, dict] = {}
    with open(args.file) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("!"):
                continue
            m = re.match(
                r'(\w[\w\d_\[\]]*)\s*=\s*'
                r'([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)'
                r'(?:\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?))?'
                r'(?:\s*[#!].*)?$',
                line,
            )
            if m:
                key = m.group(1)
                val = float(m.group(2))
                esd = float(m.group(3)) if m.group(3) else None
                params[key] = {"value": val, "esd": esd}

    if args.json:
        print(json.dumps(params, indent=2))
    else:
        for key, data in params.items():
            esd_str = f" ± {data['esd']}" if data["esd"] is not None else ""
            print(f"  {key} = {data['value']}{esd_str}")


def cmd_results(args: argparse.Namespace) -> None:
    """
    Extract refinement results from a completed Profex project.

    This reads the BGMN .par output files from a Profex project
    directory and produces structured output.
    """
    project_path = Path(args.project)
    if not project_path.exists():
        print(f"Error: Project not found: {args.project}", file=sys.stderr)
        sys.exit(1)

    # Look for output .par files in the project directory
    project_dir = project_path.parent
    base = project_path.stem
    par_files = list(project_dir.glob(f"{base}*.par"))

    if not par_files:
        print(f"Warning: No .par files found for project '{base}'", file=sys.stderr)
        if args.json:
            print(json.dumps({"status": "no_results", "project_file": str(project_path)}))
        return

    results = {
        "project_file": str(project_path),
        "status": "completed",
        "par_files": [str(p) for p in par_files],
    }

    if args.json:
        print(json.dumps(results, indent=2))
    else:
        print(f"Project: {project_path}")
        print(f"Status: completed")
        for pf in par_files:
            print(f"  Parameter file: {pf}")


def main() -> None:
    global _json_output

    parser = argparse.ArgumentParser(
        description="Profex Headless CLI — XRD analysis from the command line",
        add_help=True)
    parser.add_argument("--json", action="store_true", help="Output in JSON format")
    sub = parser.add_subparsers(dest="command", required=True)

    # convert
    p_conv = sub.add_parser("convert", help="Convert XRD data file to XY format")
    p_conv.add_argument("input", type=str, help="Input file (any supported format)")
    p_conv.add_argument("output", type=str, nargs="?", default=None, help="Output file (.xy)")
    p_conv.set_defaults(func=cmd_convert)

    # info
    p_info = sub.add_parser("info", help="Display Profex information")
    p_info.add_argument("topic", type=str, nargs="?", default="supported-formats",
                        help="Info topic (e.g. supported-formats)")
    p_info.set_defaults(func=cmd_info)

    # parse-params
    p_pp = sub.add_parser("parse-params", help="Parse a BGMN .par parameter file")
    p_pp.add_argument("file", type=str, help="Path to .par file")
    p_pp.set_defaults(func=cmd_parse_params)

    # results
    p_res = sub.add_parser("results", help="Extract refinement results from a project")
    p_res.add_argument("project", type=str, help="Path to .pro project file")
    p_res.set_defaults(func=cmd_results)

    args = parser.parse_args()
    _json_output = args.json
    args.func(args)


if __name__ == "__main__":
    main()
