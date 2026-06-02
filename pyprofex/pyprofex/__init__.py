# Profex Python 包 — pyprofex
# 暴露 XRD 计算函数和 BGMN 分析工具到 Python

from __future__ import annotations
import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import Optional

__version__ = "0.1.0"


# ─── Dataclasses for XRD Data ─────────────────────────────────────────

@dataclass
class Scan:
    """A single XRD scan (diffraction pattern)."""
    name: str
    angle: list[float] = field(default_factory=list)
    intensity: list[float] = field(default_factory=list)
    wavelength: float = 1.54056  # Cu Ka1 default
    source_file: str = ""

    @property
    def max_intensity(self) -> float:
        return max(self.intensity) if self.intensity else 0.0

    @property
    def min_intensity(self) -> float:
        return min(self.intensity) if self.intensity else 0.0

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class PhaseResult:
    """Rietveld refinement result for a single phase."""
    name: str
    weight_fraction: float
    weight_fraction_esd: float
    crystal_system: str = ""
    space_group: str = ""
    a: float = 0.0
    b: float = 0.0
    c: float = 0.0
    alpha: float = 90.0
    beta: float = 90.0
    gamma: float = 90.0
    r_bragg: float = 0.0

    def to_dict(self) -> dict:
        return asdict(self)


@dataclass
class RefinementResult:
    """Complete Rietveld refinement result."""
    phases: list[PhaseResult] = field(default_factory=list)
    rwp: float = 0.0
    rexp: float = 0.0
    gof: float = 0.0
    durbin_watson: float = 0.0
    status: str = "unknown"
    project_file: str = ""

    def to_dict(self) -> dict:
        return {
            "phases": [p.to_dict() for p in self.phases],
            "rwp": self.rwp,
            "rexp": self.rexp,
            "gof": self.gof,
            "durbin_watson": self.durbin_watson,
            "status": self.status,
            "project_file": self.project_file,
        }


# ─── XRD Calculation Utilities ─────────────────────────────────────────

def d_to_two_theta(d: float, wavelength: float = 1.54056) -> float:
    """
    Convert d-spacing (Angstrom) to 2-theta (degrees).

    Args:
        d: d-spacing in Angstrom
        wavelength: X-ray wavelength in Angstrom (default: Cu Ka1)

    Returns:
        2-theta angle in degrees
    """
    import math
    if d <= 0:
        raise ValueError("d-spacing must be positive")
    return 2.0 * math.degrees(math.asin(wavelength / (2.0 * d)))


def two_theta_to_d(two_theta: float, wavelength: float = 1.54056) -> float:
    """
    Convert 2-theta (degrees) to d-spacing (Angstrom).

    Args:
        two_theta: 2-theta angle in degrees
        wavelength: X-ray wavelength in Angstrom (default: Cu Ka1)

    Returns:
        d-spacing in Angstrom
    """
    import math
    theta_rad = math.radians(two_theta / 2.0)
    return wavelength / (2.0 * math.sin(theta_rad))


def wavelength_to_energy(wavelength: float) -> float:
    """
    Convert X-ray wavelength (Angstrom) to photon energy (keV).
    """
    return 12.39842 / wavelength


def energy_to_wavelength(energy_kev: float) -> float:
    """
    Convert photon energy (keV) to X-ray wavelength (Angstrom).
    """
    return 12.39842 / energy_kev


# ─── BGMN Parameter Parsing ─────────────────────────────────────────

def parse_bgmn_param_file(filepath: str | Path) -> dict:
    """
    Parse a BGMN .par parameter file into a Python dict.

    BGMN .par files use the format:  key = value [esd]
    Example:  a = 5.4307  0.0002   # lattice parameter a of Si
    """
    params: dict[str, dict] = {}
    with open(filepath) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or line.startswith("!"):
                continue

            m = re.match(
                r'(\w[\w\d_]*)'       # key
                r'\s*=\s*'
                r'([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?)'  # value
                r'(?:\s+([+-]?\d+\.?\d*(?:[eE][+-]?\d+)?))?'  # optional esd
                r'(?:\s*#.*)?$',  # optional comment
                line,
            )
            if m:
                key = m.group(1)
                val = float(m.group(2))
                esd = float(m.group(3)) if m.group(3) else None
                params[key] = {"value": val, "esd": esd}

    return params


# ─── Profex Command Wrapper ─────────────────────────────────────────

class ProfexCLI:
    """
    Wrapper around the Profex command-line tools.

    Requires the pxanytoxy and pxapplypreset executables built from
    the cmdtools/ directory.
    """

    def __init__(self, profex_home: str | Path | None = None):
        if profex_home is None:
            profex_home = os.environ.get("PROFEX_HOME", ".")
        self.profex_home = Path(profex_home)

        self._pxanytoxy = self.profex_home / "cmdtools" / "pxanytoxy" / "pxanytoxy"
        self._pxapplypreset = self.profex_home / "cmdtools" / "pxapplypreset" / "pxapplypreset"

    def convert_file(self, input_file: str | Path, output_file: str | Path | None = None) -> str:
        """
        Convert an XRD data file to XY format using pxanytoxy.

        Args:
            input_file: Path to input XRD file (any supported format)
            output_file: Path to output file (defaults to input basename + .xy)

        Returns:
            Path to the converted file as string
        """
        if output_file is None:
            output_file = Path(input_file).with_suffix(".xy")

        cmd = [str(self._pxanytoxy), "-i", str(input_file), "-o", str(output_file)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"pxanytoxy failed: {result.stderr}")
        return str(output_file)

    def apply_preset(self, project_file: str | Path, preset_name: str) -> str:
        """
        Apply a refinement preset to a Profex project.

        Args:
            project_file: Path to .pro project file
            preset_name: Name of the preset to apply

        Returns:
            Command output as string
        """
        cmd = [str(self._pxapplypreset), str(project_file), preset_name]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            raise RuntimeError(f"pxapplypreset failed: {result.stderr}")
        return result.stdout
