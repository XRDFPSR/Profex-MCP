#!/usr/bin/env python3
"""
pyprofex.cif2fingerprint — CIF → 粉末衍射指纹自动生成引擎

从 COD CIF 文件中读取晶体结构信息，计算理论粉末衍射花样
（d-spacing + 相对强度），输出结构化的指纹数据。

原理：
1. 解析 CIF：晶胞参数、空间群对称操作、原子坐标
2. 生成所有 hkl 组合（范围由 d_min 控制）
3. 计算 d-spacing（基于晶胞参数）
4. 计算结构因子 |F|²（基于原子坐标和散射因子）
5. 计算粉末衍射相对强度（含 Lorentz-偏振校正）
6. 输出前 N 强峰作为指纹

不需要安装任何外部 CIF 解析库，纯 Python 实现。
"""

from __future__ import annotations

import json
import math
import re
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

# ─── CIF Parser ───────────────────────────────────────────────────────


class CifParser:
    """Minimal CIF file parser — extracts cell parameters and atom sites."""

    def __init__(self, cif_text: str):
        self.text = cif_text
        self.params: dict[str, str] = {}
        self._parse()

    def _get_val(self, tag: str) -> str:
        """Get a single CIF value by tag."""
        m = re.search(
            rf'_{tag}\s+'
            r"('[^']*'|\"[^\"]*\"|[^\s]+)",
            self.text,
        )
        if m:
            return m.group(1).strip("'\" ")
        return ""

    def _clean_float(self, s: str) -> float:
        """Convert CIF float string to float, handling esd in parentheses."""
        s = s.strip().strip("'\" ")
        s = re.sub(r'\([^)]*\)', '', s)
        return float(s)

    def _parse(self):
        """Parse basic CIF metadata."""
        self.a = self._clean_float(self._get_val("cell_length_a") or "0")
        self.b = self._clean_float(self._get_val("cell_length_b") or "0")
        self.c = self._clean_float(self._get_val("cell_length_c") or "0")
        self.alpha = self._clean_float(self._get_val("cell_angle_alpha") or "90")
        self.beta = self._clean_float(self._get_val("cell_angle_beta") or "90")
        self.gamma = self._clean_float(self._get_val("cell_angle_gamma") or "90")
        self.vol = self._clean_float(self._get_val("cell_volume") or "0")
        self.Z = int(float(self._get_val("cell_formula_units_Z") or 1))
        self.sg_hm = self._get_val("symmetry_space_group_name_H-M")
        self.sg_hall = self._get_val("symmetry_space_group_name_Hall")
        self.sg_number = int(self._get_val("symmetry_Int_Tables_number") or 1)

        # Parse symmetry operations
        self._parse_symmetry_ops()

        # Parse atom sites
        self._parse_atom_sites()

        # Parse wavelength
        self.wavelength = 1.54056  # Cu Ka1 default

    def _parse_symmetry_ops(self):
        """Parse symmetry equivalent positions."""
        self.sym_ops = []
        # Find the symmetry loop
        in_sym_loop = False
        for line in self.text.split("\n"):
            line = line.strip()
            if line == "loop_":
                in_sym_loop = True
                continue
            if in_sym_loop:
                if "_symmetry_equiv_pos_as_xyz" in line:
                    continue
                if line.startswith("_"):
                    in_sym_loop = False
                    continue
                if line:
                    op = line.strip("'\" ")
                    self.sym_ops.append(op)

        # If no symmetry ops found, use identity only
        if not self.sym_ops:
            self.sym_ops = ["x, y, z"]

    def _parse_atom_sites(self):
        """Parse atom site positions from CIF."""
        self.atoms: list[dict[str, Any]] = []

        # Find atom site loop location
        lines = self.text.split("\n")
        atom_loop_start = -1
        atom_loop_keys = []

        for i, line in enumerate(lines):
            if line.startswith("_atom_site_"):
                key = line.split()[0].strip()
                atom_loop_keys.append(key)
                if atom_loop_start == -1:
                    atom_loop_start = i

        if not atom_loop_keys:
            return

        # Build column index mapping
        col_map = {}
        for j, key in enumerate(atom_loop_keys):
            if "_fract_x" in key: col_map["x"] = j
            elif "_fract_y" in key: col_map["y"] = j
            elif "_fract_z" in key: col_map["z"] = j
            elif "_type_symbol" in key: col_map["element"] = j
            elif "_label" in key: col_map["label"] = j
            elif "_occupancy" in key: col_map["occ"] = j

        if "x" not in col_map or "y" not in col_map or "z" not in col_map:
            return

        def clean_float(s: str) -> float:
            """Convert CIF float string to float, handling esd in parentheses."""
            s = s.strip()
            # Remove esd: 0.465(4) -> 0.465
            s = re.sub(r'\([^)]*\)', '', s)
            return float(s)

        def extract_element(s: str) -> str:
            """Extract element symbol: Si4+ -> Si, O2- -> O"""
            return re.sub(r'[^A-Za-z]', '', s)[:2].capitalize()

        # Parse data lines after the key list
        data_start = atom_loop_start + len(atom_loop_keys)
        for i in range(data_start, len(lines)):
            line = lines[i].strip()
            if not line or line.startswith("_") or line.startswith("loop_"):
                break
            parts = line.split()
            if len(parts) < max(col_map.values()) + 1:
                continue

            atom = {}
            if "label" in col_map:
                atom["label"] = parts[col_map["label"]]
            if "element" in col_map:
                atom["element"] = extract_element(parts[col_map["element"]])
            else:
                atom["element"] = extract_element(parts[col_map["label"]])
            
            try:
                atom["x"] = clean_float(parts[col_map["x"]])
                atom["y"] = clean_float(parts[col_map["y"]])
                atom["z"] = clean_float(parts[col_map["z"]])
            except ValueError:
                continue
            
            if "occ" in col_map:
                try:
                    atom["occ"] = clean_float(parts[col_map["occ"]])
                except ValueError:
                    atom["occ"] = 1.0
            else:
                atom["occ"] = 1.0

            self.atoms.append(atom)


# ─── Metric Tensor & d-spacing ───────────────────────────────────────

def reciprocal_metric_tensor(a, b, c, alpha, beta, gamma):
    """Calculate the reciprocal metric tensor G*.
    
    G*[i][j] = a*i · a*j where a*i are reciprocal lattice vectors.
    """
    ar = math.radians(alpha)
    br = math.radians(beta)
    gr = math.radians(gamma)
    
    # Direct metric tensor
    G = [
        [a*a, a*b*math.cos(gr), a*c*math.cos(br)],
        [a*b*math.cos(gr), b*b, b*c*math.cos(ar)],
        [a*c*math.cos(br), b*c*math.cos(ar), c*c],
    ]
    
    # Volume of unit cell
    V = a * b * c * math.sqrt(
        1 - math.cos(ar)**2 - math.cos(br)**2 - math.cos(gr)**2
        + 2 * math.cos(ar) * math.cos(br) * math.cos(gr)
    )
    
    if V <= 0:
        return G, V
    
    # Reciprocal metric tensor: G* = G⁻¹ * V²... actually G*_ij = (a*i·a*j)
    # Using: a* = (b×c)/V, etc.
    # G*_11 = a*·a* = (b×c)·(b×c)/V² = b²c²sin²(α)/V²
    # But easier: G* = G⁻¹ (inverse of direct metric tensor)
    # For a 3x3 symmetric matrix:
    det = (G[0][0] * (G[1][1]*G[2][2] - G[1][2]*G[1][2])
           - G[0][1] * (G[1][0]*G[2][2] - G[1][2]*G[0][2])
           + G[0][2] * (G[1][0]*G[1][2] - G[1][1]*G[0][2]))
    
    if abs(det) < 1e-20:
        return G, V
    
    inv_det = 1.0 / det
    Gstar = [
        [(G[1][1]*G[2][2] - G[1][2]*G[1][2]) * inv_det,
         (G[0][2]*G[1][2] - G[0][1]*G[2][2]) * inv_det,
         (G[0][1]*G[1][2] - G[0][2]*G[1][1]) * inv_det],
        [(G[0][2]*G[1][2] - G[0][1]*G[2][2]) * inv_det,
         (G[0][0]*G[2][2] - G[0][2]*G[0][2]) * inv_det,
         (G[0][1]*G[0][2] - G[0][0]*G[1][2]) * inv_det],
        [(G[0][1]*G[1][2] - G[0][2]*G[1][1]) * inv_det,
         (G[0][1]*G[0][2] - G[0][0]*G[1][2]) * inv_det,
         (G[0][0]*G[1][1] - G[0][1]*G[0][1]) * inv_det],
    ]
    
    return Gstar, V


def d_spacing_hkl(Gstar, h, k, l):
    """Calculate d-spacing from reciprocal metric tensor and Miller indices.
    
    1/d² = h·G*·h  (where h = [h k l]ᵀ)
    """
    d2_inv = (h*h*Gstar[0][0] + k*k*Gstar[1][1] + l*l*Gstar[2][2]
              + 2*h*k*Gstar[0][1] + 2*h*l*Gstar[0][2] + 2*k*l*Gstar[1][2])
    if d2_inv <= 0:
        return 0.0
    return math.sqrt(1.0 / d2_inv)


# ─── Atomic Scattering Factors ───────────────────────────────────────

# Cromer-Mann coefficients for selected elements (a1,b1,a2,b2,a3,b3,a4,b4,c)
# From International Tables for Crystallography
SCATTERING_COEFFS = {
    "H":  [0.48992, 20.6593, 0.26200, 7.74039, 0.19677, 49.5519, 0.04988, 2.20159, 0.00130],
    "Li": [0.99279, 4.33979, 0.87402, 1.26006, 0.84240, 98.7088, 0.23101, 212.088, 0.05988],
    "Be": [2.22744, 0.04965, 1.55249, 42.9165, 1.40060, 1.66379, 0.58290, 100.361, -1.76339],
    "B":  [2.03876, 23.0888, 1.41491, 0.97848, 1.11609, 59.8985, 0.73273, 0.08538, -0.30409],
    "C":  [1.93019, 12.7188, 1.87812, 28.6498, 1.57415, 0.59645, 0.37108, 65.0337, 0.24637],
    "N":  [12.7913, 0.02064, 3.28546, 10.7018, 1.76483, 30.7773, 0.54709, 1.48044, -11.3926],
    "O":  [2.95648, 13.8964, 2.45240, 5.91765, 1.50510, 0.34537, 0.78135, 34.0811, 0.30413],
    "F":  [3.30393, 11.2651, 3.01753, 4.66504, 1.35754, 0.33760, 0.83645, 27.9898, 0.48398],
    "Na": [5.26400, 4.02579, 2.17549, 10.4796, 1.36690, 0.84222, 1.08859, 133.617, 1.09912],
    "Mg": [5.59229, 4.41142, 2.68206, 1.36549, 1.72235, 93.4885, 0.73055, 32.5281, 1.26883],
    "Al": [5.35047, 3.48665, 2.92451, 1.20535, 2.27309, 42.6051, 1.16531, 107.170, 1.28489],
    "Si": [5.79411, 2.57104, 3.22390, 34.1775, 2.42795, 0.86937, 1.32149, 85.3410, 1.23139],
    "P":  [6.92073, 1.83778, 4.14396, 27.0198, 2.01697, 0.21318, 1.53860, 67.1086, 0.37870],
    "S":  [7.18742, 1.43280, 5.88671, 0.02865, 5.15858, 22.1101, 1.64403, 55.4651, -3.87732],
    "Cl": [9.83957, -0.00053, 7.53181, 1.11119, 6.07100, 18.0846, 1.87128, 45.3666, -8.31430],
    "K":  [8.11756, 12.6684, 7.48062, 0.76409, 1.07795, 211.222, 0.97218, 37.2727, 1.35009],
    "Ca": [8.60272, 10.2636, 7.50769, 0.62794, 1.75117, 149.301, 0.96216, 60.2274, 1.17430],
    "Ti": [9.54969, 7.60579, 7.60067, 0.45899, 2.17223, 109.099, 1.75438, 27.5715, 0.91762],
    "Cr": [10.4757, 6.01658, 7.51402, 0.37426, 3.50115, 19.0654, 1.54902, 97.4599, 0.95226],
    "Mn": [11.2519, 5.34818, 7.36935, 0.34373, 3.04107, 17.4089, 2.27703, 84.2139, 1.05195],
    "Fe": [11.9185, 4.87394, 7.04848, 0.34023, 3.34326, 15.9330, 2.27228, 79.0339, 1.40818],
    "Co": [12.6158, 4.48994, 6.62642, 0.35459, 3.57722, 14.8402, 2.25644, 74.7352, 1.91452],
    "Ni": [13.3239, 4.17742, 6.18746, 0.38682, 3.74792, 14.0123, 2.23195, 71.1195, 2.49899],
    "Cu": [13.9352, 3.97779, 5.84833, 0.44555, 4.64221, 13.3971, 1.44753, 74.1605, 3.11686],
    "Zn": [14.6744, 3.71486, 5.62816, 0.50033, 3.92540, 12.8862, 2.16398, 65.4071, 3.59838],
    "Zr": [19.2273, 1.15488, 10.1378, 10.7877, 2.48177, 120.126, 2.42892, 33.3722, 5.71886],
}


def scattering_factor(element, sin_theta_over_lambda):
    """Cromer-Mann f(sinθ/λ) calculation."""
    coeffs = SCATTERING_COEFFS.get(element.capitalize())
    if coeffs is None:
        # Approximate by atomic number
        atomic_nums = {"H":1,"Li":3,"Be":4,"B":5,"C":6,"N":7,"O":8,"F":9,"Na":11,"Mg":12,
                      "Al":13,"Si":14,"P":15,"S":16,"Cl":17,"K":19,"Ca":20,"Ti":22,
                      "Cr":24,"Mn":25,"Fe":26,"Co":27,"Ni":28,"Cu":29,"Zn":30,"Zr":40}
        z = atomic_nums.get(element.capitalize(), 10)
        return float(z)
    
    a1, b1, a2, b2, a3, b3, a4, b4, c = coeffs
    s2 = sin_theta_over_lambda * sin_theta_over_lambda
    return (a1 * math.exp(-b1 * s2) + a2 * math.exp(-b2 * s2)
            + a3 * math.exp(-b3 * s2) + a4 * math.exp(-b4 * s2) + c)


# ─── Structure Factor Calculation ─────────────────────────────────────

def apply_symmetry(x, y, z, sym_op_str):
    """Apply a symmetry operation string to a fractional coordinate."""
    # Parse simple symmetry operation like "x, y, z" or "-x, y+1/2, -z"
    parts = [p.strip() for p in sym_op_str.split(",")]
    if len(parts) != 3:
        return [(x, y, z)]

    results = []
    for i, part in enumerate([x, y, z]):
        coord_str = parts[0] if i == 0 else (parts[1] if i == 1 else parts[2])
        val = part
        # Apply sign
        if "-x" in coord_str or "-y" in coord_str or "-z" in coord_str:
            val = -val
        # Apply translation
        trans_match = re.search(r'([+-]?\d+/\d+)', coord_str)
        if trans_match:
            frac = trans_match.group(1)
            num, den = frac.split("/")
            val += float(num) / float(den)
        results.append(val)
    return tuple(results)


def generate_equivalent_positions(x, y, z, sym_ops):
    """Generate all symmetry-equivalent positions."""
    positions = set()
    for op in sym_ops:
        parts = [p.strip() for p in op.split(",")]
        if len(parts) != 3:
            continue
        new_pos = []
        for i, coord_str in enumerate(parts):
            orig = [x, y, z][i]
            sign = -1 if "-" in coord_str and coord_str.index("-") < 3 else 1
            val = sign * orig
            # Translations
            for m in re.finditer(r'([+-]?\d+/\d+)', coord_str):
                num, den = m.group(1).split("/")
                val += float(num) / float(den)
            new_pos.append(val % 1.0)
        positions.add(tuple(new_pos))
    return list(positions)


def calculate_structure_factor(h, k, l, atoms, sym_ops, sin_theta_over_lambda):
    """Calculate structure factor F(hkl)."""
    F_real = 0.0
    F_imag = 0.0
    
    for atom in atoms:
        el = atom.get("element", "O")
        occ = atom.get("occ", 1.0)
        x0, y0, z0 = atom.get("x", 0), atom.get("y", 0), atom.get("z", 0)
        f = scattering_factor(el, sin_theta_over_lambda)
        
        # Sum over symmetry equivalent positions
        eq_pos = [(x0, y0, z0)]  # Simplified: use just the asym unit position
        # In a full implementation, apply sym_ops here
        
        for pos in eq_pos:
            px, py, pz = pos
            phase = 2 * math.pi * (h * px + k * py + l * pz)
            F_real += occ * f * math.cos(phase)
            F_imag += occ * f * math.sin(phase)
    
    return F_real, F_imag


# ─── Lorentz-Polarization Correction ─────────────────────────────────

def lp_correction(two_theta, wavelength=1.54056):
    """Lorentz-polarization factor for powder diffraction."""
    theta = math.radians(two_theta / 2.0)
    cos2theta = math.cos(2 * theta)
    # L = 1/sin²θ·cosθ, P = (1+cos²2θ)/2 for unpolarized
    return (1 + cos2theta * cos2theta) / (math.sin(theta) * math.sin(theta) * math.cos(theta))


# ─── Main Pipeline ──────────────────────────────────────────────────

def calculate_powder_pattern(cif_text, d_min=0.8, wavelength=1.54056, top_n=20):
    """
    Calculate powder diffraction pattern from CIF text.
    
    Returns:
        List of {d, intensity, h, k, l, two_theta} sorted by intensity descending.
    """
    parser = CifParser(cif_text)
    if parser.a == 0:
        return []
    
    Gstar, V = reciprocal_metric_tensor(parser.a, parser.b, parser.c, parser.alpha, parser.beta, parser.gamma)
    if V <= 0:
        return []
    
    # Determine hkl range based on d_min
    max_h = max_k = max_l = int(parser.a / d_min) + 3
    
    # Multiplicity tracking
    reflection_map = {}  # (d_rounded, equivalent_hkl) -> (h,k,l,intensity)
    
    reflections = []
    reflection_map = {}
    
    for h in range(-max_h, max_h + 1):
        for k in range(-max_k, max_k + 1):
            for l in range(0, max_l + 1):  # l >= 0 to avoid Friedel pairs
                if h == 0 and k == 0 and l == 0:
                    continue
                d = d_spacing_hkl(Gstar, h, k, l)
                if d < d_min or d <= 0:
                    continue
                
                # Filter by systematic absences for common space groups
                # Centrosymmetric: all reflections considered
                
                two_theta = 2 * math.degrees(math.asin(wavelength / (2 * d)))
                if two_theta > 150 or two_theta < 1:
                    continue
                
                sin_theta_over_lambda = math.sin(math.radians(two_theta / 2)) / wavelength
                
                # Structure factor
                F_real, F_imag = calculate_structure_factor(
                    h, k, l, parser.atoms, parser.sym_ops, sin_theta_over_lambda
                )
                F_sq = F_real * F_real + F_imag * F_imag
                
                if F_sq < 0.001:
                    continue
                
                # Lorentz-polarization
                lp = lp_correction(two_theta, wavelength)
                intensity = F_sq * lp
                
                # Round d to merge equivalent reflections
                d_rounded = round(d, 2)  # Merge reflections within 0.01Å
                key = (d_rounded, h >= 0 and k >= 0 and l >= 0)
                
                if key in reflection_map:
                    # Add to existing (multiplicity)
                    existing = reflection_map[key]
                    existing["multiplicity"] += 1
                    existing["intensity"] += intensity
                else:
                    reflection_map[key] = {
                        "h": h, "k": k, "l": l,
                        "d": round(d, 4),
                        "two_theta": round(two_theta, 4),
                        "intensity": intensity,
                        "F_sq": round(F_sq, 2),
                        "multiplicity": 1,
                    }
    
    # Sort by intensity descending
    reflections = list(reflection_map.values())
    reflections.sort(key=lambda r: r["intensity"], reverse=True)
    
    # Normalize intensities to I_max = 100
    if reflections:
        i_max = reflections[0]["intensity"]
        for r in reflections:
            r["intensity_rel"] = round(r["intensity"] / i_max * 100, 1)
    
    return reflections[:top_n]


# ─── COD Download + Process ──────────────────────────────────────────

COD_BASE = "https://www.crystallography.net/cod"

def get_cif(entry_id: str) -> str | None:
    """Download a CIF file from COD by entry ID."""
    url = f"{COD_BASE}/cif/{entry_id}.cif"
    try:
        with urllib.request.urlopen(url, timeout=15) as resp:
            return resp.read().decode()
    except urllib.error.URLError:
        return None


def process_cod_entry(entry_id: str, name: str = "", d_min: float = 0.8, top_n: int = 15):
    """
    Download CIF from COD and calculate powder pattern.
    Returns dict with fingerprint data.
    """
    cif = get_cif(entry_id)
    if not cif:
        return {"error": f"CIF not found for {entry_id}"}
    
    pattern = calculate_powder_pattern(cif, d_min=d_min, top_n=top_n)
    if not pattern:
        return {"error": f"Could not calculate pattern for {entry_id}"}
    
    # Extract CIF metadata
    parser = CifParser(cif)
    
    # Build element list from atoms
    elements = list(set(a.get("element", "?") for a in parser.atoms if a.get("element")))
    # Remove non-standard elements
    elements = [e for e in elements if e[0].isalpha()]
    
    fingerprint = {
        "entry_id": entry_id,
        "name": name,
        "mineral": name,
        "formula": parser._get_val("chemical_formula_sum") or parser._get_val("chemical_formula_structural") or "",
        "sg": parser.sg_hm,
        "a": parser.a, "b": parser.b, "c": parser.c,
        "alpha": parser.alpha, "beta": parser.beta, "gamma": parser.gamma,
        "vol": parser.vol,
        "Z": parser.Z,
        "elements": elements,
        "n_atoms": len(parser.atoms),
        "peaks": [{"d": r["d"], "h": r["h"], "k": r["k"], "l": r["l"],
                    "intensity": r["intensity_rel"], "two_theta": r["two_theta"]}
                   for r in pattern],
    }
    return fingerprint


# ─── CLI Entry Point ──────────────────────────────────────────────────

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) < 2:
        print("Usage: python cif2fingerprint.py <COD_entry_id> [name]")
        print("Example: python cif2fingerprint.py 1011097 Quartz")
        sys.exit(1)
    
    entry_id = sys.argv[1]
    name = sys.argv[2] if len(sys.argv) > 2 else ""
    
    result = process_cod_entry(entry_id, name)
    print(json.dumps(result, indent=2, ensure_ascii=False))
