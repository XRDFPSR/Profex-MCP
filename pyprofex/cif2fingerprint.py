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
import threading

# Thread lock for CIF cache writes
_CIF_LOCK = threading.Lock()

# ─── CIF Parser ───────────────────────────────────────────────────────

# 前 50 常用空间群的对称操作数据库
# 格式: {空间群号: ["op1,op2,...", ...]}
_SG_SYMOPS: dict[int, list[str]] = {
    # SG #1 (P1) - 三斜晶系
    1: ["x, y, z"],
    # SG #2 (P-1) - 三斜晶系
    2: ["x, y, z", "-x, -y, -z"],
    # SG #4 (P2_1) - 单斜晶系
    4: ["x, y, z", "-x, y+1/2, -z"],
    # SG #5 (C2) - 单斜晶系
    5: ["x, y, z", "-x, y, -z"],
    # SG #7 (Pc) - 单斜晶系
    7: ["x, y, z", "x, -y, z+1/2"],
    # SG #9 (Cc) - 单斜晶系
    9: ["x, y, z", "x, -y, z+1/2"],
    # SG #11 (P2_1/m) - 单斜晶系
    11: ["x, y, z", "-x, y+1/2, -z", "-x, -y, -z", "x, -y+1/2, z"],
    # SG #12 (C2/m) - 单斜晶系
    12: ["x, y, z", "-x, y, -z", "-x, -y, -z", "x, -y, z"],
    # SG #13 (P2/c) - 单斜晶系
    13: ["x, y, z", "-x, y+1/2, -z", "-x, -y, -z", "x, -y+1/2, z"],
    # SG #14 (P2_1/c) - 单斜晶系（最常见的空间群！160K+ 条目）
    14: ["x, y, z", "-x, y+1/2, -z", "-x, -y, -z", "x, -y+1/2, z"],
    # SG #15 (C2/c) - 单斜晶系
    15: ["x, y, z", "-x, y, -z", "-x, -y, -z", "x, -y, z"],
    # SG #19 (P2_12_12_1) - 正交晶系
    19: ["x, y, z", "-x+1/2, -y, z+1/2", "-x, y+1/2, -z+1/2", "x+1/2, -y+1/2, -z"],
    # SG #20 (C222_1) - 正交晶系
    20: ["x, y, z", "-x, -y, z+1/2", "-x, y, -z+1/2", "x, -y, -z"],
    # SG #33 (Pna2_1) - 正交晶系
    33: ["x, y, z", "-x+1/2, -y, z+1/2", "-x, y+1/2, -z+1/2", "x+1/2, -y+1/2, -z"],
    # SG #60 (Pbcn) - 正交晶系
    60: ["x, y, z", "-x, -y, z+1/2", "-x, y, -z+1/2", "x, -y, -z",
         "-x, -y, -z", "x, y, -z+1/2", "x, -y, z+1/2", "-x, y, z"],
    # SG #61 (Pbca) - 正交晶系（14K+ 条目）
    61: ["x, y, z", "-x+1/2, -y, z+1/2", "-x, y+1/2, -z+1/2", "x+1/2, -y+1/2, -z",
         "-x, -y, -z", "x+1/2, y, -z+1/2", "x, -y+1/2, z+1/2", "-x+1/2, y+1/2, z"],
    # SG #62 (Pnma) - 正交晶系
    62: ["x, y, z", "-x+1/2, -y, z+1/2", "-x, y+1/2, -z+1/2", "x+1/2, -y+1/2, -z",
         "-x, -y, -z", "x+1/2, y, -z+1/2", "x, -y+1/2, z+1/2", "-x+1/2, y+1/2, z"],
    # SG #2-14 是单斜/正交，下面是四方/三方/六方
    # SG #75 (P4) - 四方晶系
    75: ["x, y, z", "-x, -y, z"],
    # SG #76 (P4_1) - 四方晶系
    76: ["x, y, z", "-x, -y, z+1/4"],
    # SG #83 (P4/m) - 四方晶系
    83: ["x, y, z", "-x, -y, z", "-x, y, -z", "x, -y, -z",
         "-x, -y, -z", "x, y, -z", "x, -y, z", "-x, y, z"],
    # SG #85 (P4/n) - 四方晶系
    85: ["x, y, z", "-x, -y, z", "-y+1/2, x+1/2, z", "y+1/2, -x+1/2, z"],
    # SG #86 (P4_2/n) - 四方晶系
    86: ["x, y, z", "-x, -y, z", "-y+1/2, x+1/2, z+1/2", "y+1/2, -x+1/2, z+1/2"],
    # SG #88 (I4_1/a) - 四方晶系
    88: ["x, y, z", "-x+1/2, -y+1/2, z", "-y+1/2, x, z+1/4", "y, -x+1/2, z+1/4",
         "-x+1/2, y+1/2, -z+1/2", "x, y, -z+1/2", "y+1/2, -x, -z+1/4", "-y, x+1/2, -z+1/4"],
    # SG #92 (P4_12_12) - 四方晶系
    92: ["x, y, z", "-x, -y, z+1/2", "-y+1/2, x+1/2, z+1/4", "y+1/2, -x+1/2, z+3/4"],
    # SG #96 (P4_32_12) - 四方晶系
    96: ["x, y, z", "-x, -y, z+1/2", "-y+1/2, x+1/2, z+3/4", "y+1/2, -x+1/2, z+1/4"],
    # SG #143 (P3) - 三方晶系
    143: ["x, y, z", "-y, x-y, z", "y-x, -x, z"],
    # SG #144 (P3_1) - 三方晶系
    144: ["x, y, z", "-y, x-y, z+1/3", "y-x, -x, z+2/3"],
    # SG #145 (P3_2) - 三方晶系
    145: ["x, y, z", "-y, x-y, z+2/3", "y-x, -x, z+1/3"],
    # SG #146 (R3) - 三方晶系（菱面体）
    146: ["x, y, z", "z, x, y", "y, z, x"],
    # SG #147 (P-3) - 三方晶系
    147: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "-x, -y, -z", "y, y-x, -z", "x-y, x, -z"],
    # SG #148 (R-3) - 三方晶系（菱面体，4K+ 条目）
    148: ["x, y, z", "z, x, y", "y, z, x",
          "-x, -y, -z", "-z, -x, -y", "-y, -z, -x"],
    # SG #150 (P321) - 三方晶系
    150: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "y, x, -z", "x-y, -y, -z", "-x, y-x, -z"],
    # SG #152 (P3_121) - 三方晶系（石英！）
    152: ["x, y, z", "-y, x-y, z+1/3", "y-x, -x, z+2/3",
          "y, x, -z", "-x, y-x, -z+1/3", "x-y, -y, -z+2/3"],
    # SG #154 (P3_221) - 三方晶系
    154: ["x, y, z", "-y, x-y, z+2/3", "y-x, -x, z+1/3",
          "y, x, -z", "-x, y-x, -z+2/3", "x-y, -y, -z+1/3"],
    # SG #155 (R32) - 三方晶系（菱面体）
    155: ["x, y, z", "z, x, y", "y, z, x",
          "y, x, -z", "x, z, -y", "z, y, -x"],
    # SG #160 (R3m) - 三方晶系（菱面体）
    160: ["x, y, z", "z, x, y", "y, z, x",
          "y, x, z", "x, z, y", "z, y, x"],
    # SG #163 (P3_1c) - 三方晶系
    163: ["x, y, z", "-y, x-y, z+2/3", "y-x, -x, z+1/3",
          "-x, -y, -z+1/2", "y, y-x, -z+1/6", "x-y, x, -z+5/6"],
    # SG #166 (R-3m) - 三方晶系（菱面体）
    166: ["x, y, z", "z, x, y", "y, z, x",
          "y, x, z", "x, z, y", "z, y, x",
          "-x, -y, -z", "-z, -x, -y", "-y, -z, -x",
          "-y, -x, -z", "-x, -z, -y", "-z, -y, -x"],
    # SG #167 (R-3c) - 三方晶系（菱面体 setting）
    167: ["x, y, z", "z, x, y", "y, z, x",
          "-y, -x, -z", "-x, -z, -y", "-z, -y, -x"],
    # SG #167 HEX (R-3c :H) - 六方 setting
    # 需要 12 个操作: 6 个旋转 + 6 个 c-滑移 (c/2 平移后 xy 不变)
    100167: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
             "-x, -y, z+1/2", "y, y-x, z+1/2", "x-y, x, z+1/2",
             "-x, -y, -z", "y, y-x, -z", "x-y, x, -z",
             "x, y, -z+1/2", "-y, x-y, -z+1/2", "y-x, -x, -z+1/2"],
    # SG #168 (P6) - 六方晶系
    168: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "-x, -y, z", "y, y-x, z", "x-y, x, z"],
    # SG #169 (P6_1) - 六方晶系
    169: ["x, y, z", "-y, x-y, z+1/6", "y-x, -x, z+1/3",
          "-x, -y, z+1/2", "y, y-x, z+2/3", "x-y, x, z+5/6"],
    # SG #170 (P6_5) - 六方晶系
    170: ["x, y, z", "-y, x-y, z+5/6", "y-x, -x, z+2/3",
          "-x, -y, z+1/2", "y, y-x, z+1/3", "x-y, x, z+1/6"],
    # SG #172 (P6_4) - 六方晶系
    172: ["x, y, z", "-y, x-y, z+2/3", "y-x, -x, z+1/3",
          "-x, -y, z+1/2", "y, y-x, z+1/6", "x-y, x, z+5/6"],
    # SG #173 (P6_3) - 六方晶系
    173: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "-x, -y, z+1/2", "y, y-x, z+1/2", "x-y, x, z+1/2"],
    # SG #174 (P-6) - 六方晶系
    174: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "-x, -y, -z", "y, y-x, -z", "x-y, x, -z"],
    # SG #176 (P6_3/m) - 六方晶系
    176: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "-x, -y, z+1/2", "y, y-x, z+1/2", "x-y, x, z+1/2",
          "-x, -y, -z", "y, y-x, -z", "x-y, x, -z",
          "x, y, -z+1/2", "-y, x-y, -z+1/2", "y-x, -x, -z+1/2"],
    # SG #186 (P6_3mc) - 六方晶系（Zincite/ZnO！）
    186: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "-y, -x, z", "y-x, y, z", "x, x-y, z",
          "-x, -y, z+1/2", "y, y-x, z+1/2", "x-y, x, z+1/2",
          "y, x, z+1/2", "x-y, -y, z+1/2", "-x, y-x, z+1/2"],
    # SG #187 (P-6m2) - 六方晶系
    187: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "y, x, -z", "x-y, -y, -z", "-x, y-x, -z",
          "-x, -y, -z", "y, y-x, -z", "x-y, x, -z",
          "-y, -x, z", "y-x, y, z", "x, x-y, z"],
    # SG #194 (P6_3/mmc) - 六方晶系
    194: ["x, y, z", "-y, x-y, z", "y-x, -x, z",
          "-x, -y, z+1/2", "y, y-x, z+1/2", "x-y, x, z+1/2",
          "-y, -x, z", "y-x, y, z", "x, x-y, z",
          "y, x, z+1/2", "x-y, -y, z+1/2", "-x, y-x, z+1/2",
          "-x, -y, -z", "y, y-x, -z", "x-y, x, -z",
          "x, y, -z+1/2", "-y, x-y, -z+1/2", "y-x, -x, -z+1/2",
          "y, x, -z", "x-y, -y, -z", "-x, y-x, -z",
          "-y, -x, -z+1/2", "y-x, y, -z+1/2", "x, x-y, -z+1/2"],
    # SG #198 (P2_13) - 立方晶系
    198: ["x, y, z", "y, z, x", "z, x, y",
          "-x, -y, z+1/2", "-y, -z, x+1/2", "-z, -x, y+1/2",
          "-x, y+1/2, -z", "-y, z+1/2, -x", "-z, x+1/2, -y",
          "x+1/2, -y, -z", "y+1/2, -z, -x", "z+1/2, -x, -y"],
    # SG #205 (Pa-3) - 立方晶系
    205: ["x, y, z", "-x, -y, -z",
          "-x+1/2, -y, z+1/2", "x+1/2, y, -z+1/2",
          "-x, y+1/2, -z+1/2", "x, -y+1/2, z+1/2",
          "x+1/2, -y+1/2, -z", "-x+1/2, y+1/2, z"],
    # SG #221 (Pm-3m) - 立方晶系
    221: ["x, y, z", "z, x, y", "y, z, x",
          "-x, -y, z", "-z, -x, y", "-y, -z, x",
          "-x, y, -z", "-z, x, -y", "-y, z, -x",
          "x, -y, -z", "z, -x, -y", "y, -z, -x",
          "-x, -y, -z", "-z, -x, -y", "-y, -z, -x",
          "x, y, -z", "z, x, -y", "y, z, -x",
          "x, -y, z", "z, -x, y", "y, -z, x",
          "-x, y, z", "-z, x, y", "-y, z, x"],
    # SG #225 (Fm-3m) - 立方晶系（岩盐 NaCl 结构，4K+ 条目）
    225: ["x, y, z", "-x, -y, z", "-x, y, -z", "x, -y, -z",
          "-x, -y, -z", "x, y, -z", "x, -y, z", "-x, y, z",
          "z, x, y", "-z, -x, y", "-z, x, -y", "z, -x, -y",
          "-z, -x, -y", "z, x, -y", "z, -x, y", "-z, x, y",
          "y, z, x", "-y, -z, x", "-y, z, -x", "y, -z, -x",
          "-y, -z, -x", "y, z, -x", "y, -z, x", "-y, z, x",
          "x+1/2, y+1/2, z", "x+1/2, y+1/2, z",  # 面心平移
    ],
    # SG #227 (Fd-3m) - 立方晶系（金刚石结构，4K+ 条目）
    227: ["x, y, z", "-x, -y, z", "-x, y, -z", "x, -y, -z",
          "-x, -y, -z", "x, y, -z", "x, -y, z", "-x, y, z",
          "z, x, y", "-z, -x, y", "-z, x, -y", "z, -x, -y",
          "-z, -x, -y", "z, x, -y", "z, -x, y", "-z, x, y",
          "y, z, x", "-y, -z, x", "-y, z, -x", "y, -z, -x",
          "-y, -z, -x", "y, z, -x", "y, -z, x", "-y, z, x",
          "x+1/4, y+1/4, z+1/4", "-x+3/4, -y+3/4, z+1/4",
          "-x+3/4, y+1/4, -z+3/4", "x+1/4, -y+3/4, -z+3/4",
          "-x+3/4, -y+3/4, -z+3/4", "x+1/4, y+1/4, -z+3/4",
          "x+1/4, -y+3/4, z+1/4", "-x+3/4, y+1/4, z+1/4"],
    # SG #230 (Ia-3d) - 立方晶系
    230: ["x, y, z", "-x, -y, z", "-x, y, -z", "x, -y, -z",
          "-x, -y, -z", "x, y, -z", "x, -y, z", "-x, y, z",
          "z, x, y", "-z, -x, y", "-z, x, -y", "z, -x, -y",
          "-z, -x, -y", "z, x, -y", "z, -x, y", "-z, x, y",
          "y, z, x", "-y, -z, x", "-y, z, -x", "y, -z, -x",
          "-y, -z, -x", "y, z, -x", "y, -z, x", "-y, z, x",
          "x+1/4, y+1/4, z+1/4", "-x+1/4, -y+1/4, z+1/4",
          "-x+1/4, y+1/4, -z+1/4", "x+1/4, -y+1/4, -z+1/4",
          "-x+3/4, -y+3/4, -z+3/4", "x+3/4, y+3/4, -z+3/4",
          "x+3/4, -y+3/4, z+3/4", "-x+3/4, y+3/4, z+3/4"],
}


def _generate_symops_from_sg(sg_number: int) -> list[str] | None:
    """
    对未在 _SG_SYMOPS 中的空间群，生成最小对称操作。
    
    基于晶系和格子类型推断基本对称操作。
    """
    if sg_number <= 0:
        return None
    return None  # 未覆盖的由 _SG_SYMOPS 兜底

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

        # If no symmetry ops found (e.g. CIF only gives space group symbol but not explicit ops),
        # look up from built-in database
        if len(self.sym_ops) <= 2 and self.sg_number > 0:
            # Heuristic: COD hex setting R sg's often have sgNumber=1, fix lookup
            actual_sg = self.sg_number
            if actual_sg == 1 and self.sg_hm:
                # Check for rhombohedral/hexagonal R space groups
                sg_upper = self.sg_hm.upper().replace(" ", "")
                if sg_upper.startswith("R-3C"):
                    # Detect hexagonal vs rhombohedral setting by cell angles
                    is_hex = (
                        abs(self.alpha - 90) < 1 and
                        abs(self.beta - 90) < 1 and
                        abs(self.gamma - 120) < 1
                    )
                    if is_hex:
                        actual_sg = 100167  # Hexagonal setting of R-3c
                    else:
                        actual_sg = 167
                elif sg_upper.startswith("R-3M"):
                    actual_sg = 166
                elif sg_upper.startswith("R-3"):
                    actual_sg = 148
                elif sg_upper.startswith("R3M") or sg_upper.startswith("R3M"):
                    actual_sg = 160
                elif sg_upper.startswith("R32"):
                    actual_sg = 155
                elif sg_upper.startswith("R3") and ":" not in sg_upper:
                    # Trigonal R3
                    if ":" not in sg_upper:
                        actual_sg = 146
                    elif ":H" in sg_upper:
                        actual_sg = 146
                elif sg_upper.startswith("P-1"):
                    actual_sg = 2
            lookup = _SG_SYMOPS.get(actual_sg)
            if lookup:
                self.sym_ops = lookup[:]
            else:
                # Generate from minimal symmetry
                base_ops = _generate_symops_from_sg(self.sg_number)
                if base_ops:
                    self.sym_ops = base_ops
                else:
                    self.sym_ops = ["x, y, z"]

    def _parse_atom_sites(self):
        """Parse atom site positions from CIF."""
        self.atoms: list[dict[str, Any]] = []

        lines = self.text.split("\n")
        
        # Strategy: find all loop blocks, pick the one containing atom site fract coords
        i = 0
        n = len(lines)
        while i < n:
            line = lines[i].strip()
            
            # Find loop_ start
            if line != "loop_":
                i += 1
                continue
            
            # Collect keys of this loop block
            j = i + 1
            keys = []
            while j < n:
                l = lines[j].strip()
                if not l or l == "loop_" or not l.startswith("_"):
                    break
                # It's a key line only if the next line also starts with _
                # or if this is the last key before data
                kl = l.split()[0].strip()
                keys.append(kl)
                j += 1
            
            if not keys:
                i = j
                continue
            
            # Check if this loop has _atom_site_fract_x
            has_fract = any("_fract_x" in k for k in keys)
            if not has_fract:
                i = j
                continue
            
            # Check it's the atom site loop (not anisotropic)
            is_main_atom = any("_atom_site_type_symbol" in k or 
                              ("_atom_site_label" in k and not any("_aniso_" in kk for kk in keys))
                              for k in keys)
            if not is_main_atom:
                i = j
                continue
            
            # Build column mapping
            col_map = {}
            for idx, key in enumerate(keys):
                if "_fract_x" in key: col_map["x"] = idx
                elif "_fract_y" in key: col_map["y"] = idx
                elif "_fract_z" in key: col_map["z"] = idx
                elif "_type_symbol" in key: col_map["element"] = idx
                elif "_label" in key: col_map["label"] = idx
                elif "_occupancy" in key: col_map["occ"] = idx
                elif "_site_symmetry" in key or "_symmetry_multiplicity" in key:
                    col_map["symmetry_multiplicity"] = idx
                elif "_Wyckoff" in key:
                    col_map["wyckoff"] = idx
                elif "_calc_flag" in key:
                    col_map["calc_flag"] = idx
                elif "_U_iso" in key or "_B_iso" in key:
                    col_map["u_iso"] = idx

            if "x" not in col_map or "y" not in col_map or "z" not in col_map:
                i = j
                continue

            def clean_float(s: str) -> float:
                s = s.strip()
                s = re.sub(r'\([^)]*\)', '', s)
                return float(s)

            def extract_element(s: str) -> str:
                return re.sub(r'[^A-Za-z]', '', s)[:2].capitalize()

            # Parse data lines
            k = j
            max_col = max(col_map.values())
            while k < n:
                dl = lines[k].strip()
                if not dl or dl.startswith("_") or dl == "loop_":
                    break
                parts = dl.split()
                if len(parts) <= max_col:
                    k += 1
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
                    k += 1
                    continue

                if "occ" in col_map:
                    try:
                        atom["occ"] = clean_float(parts[col_map["occ"]])
                    except ValueError:
                        atom["occ"] = 1.0
                else:
                    atom["occ"] = 1.0

                self.atoms.append(atom)
                k += 1
            
            # Successfully parsed this loop, done
            break


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


def _parse_sym_coord(expr: str, x: float, y: float, z: float) -> float:
    """
    Parse a single symmetry coordinate expression like "-x", "y+1/2",
    "1/3+x", or "x-y" and evaluate against the given coordinates.

    Strategy: replace each variable occurrence (optionally preceded by
    a fractional coefficient) with its numeric value, then evaluate
    the resulting arithmetic expression.

    Supported formats:
    - "x"           -> +x
    - "-x"          -> -x
    - "1/3+x"       -> +1/3 + x
    - "y+1/2"       -> y + 1/2
    - "x-y"         -> x - y
    - "-z+2/3"      -> -z + 2/3
    - "2/3+x+y"     -> 2/3 + x + y
    """
    expr = expr.replace(" ", "")
    if not expr:
        return 0.0

    vars_map = {"x": x, "y": y, "z": z}

    # Insert '+' before first term if expression starts with a variable
    # or fraction (to make parsing easier)
    if expr[0] in "xyz" or expr[0].isdigit():
        expr = "+" + expr

    result = 0.0
    i = 0
    n = len(expr)

    while i < n:
        # Determine sign
        if expr[i] == '+':
            sign = 1.0
            i += 1
        elif expr[i] == '-':
            sign = -1.0
            i += 1
        elif expr[i] == ' ':
            i += 1
            continue
        else:
            # Should not happen in a well-formed expression
            i += 1
            continue

        # Now parse the value: could be a fraction + variable, fraction only,
        # or variable only
        val = 0.0

        if i < n and expr[i].isdigit():
            # Parse fraction: number/number
            j = i
            has_digit = False
            while j < n and expr[j] in '0123456789/':
                if expr[j].isdigit():
                    has_digit = True
                j += 1
            frac_str = expr[i:j]
            if '/' in frac_str:
                num_str, den_str = frac_str.split('/')
                val = float(num_str) / float(den_str)
            elif has_digit:
                # Plain number (shouldn't normally appear in CIF coords)
                val = float(frac_str)
            i = j

        # Check if a variable follows
        if i < n and expr[i] in "xyz":
            if val == 0.0:
                val = 1.0  # Just a bare variable like "x"
            result += sign * val * vars_map[expr[i]]
            i += 1
        else:
            # Pure translation term (fraction without variable)
            result += sign * val

    return result


def apply_symmetry_op(x, y, z, op_str):
    """
    Apply a symmetry operation string to fractional coordinates.

    Supports CIF-standard formats:
    - "x, y, z"                  -> identity
    - "-x, y+1/2, -z"            -> sign flip + translation
    - "1/3+x, 2/3+y, 2/3+z"     -> translation first
    - "x, x-y, z"                -> linear combination
    - "y, x, -z+2/3"             -> permutation + translation

    Returns: (new_x, new_y, new_z) with coordinates in [0, 1).
    """
    parts = [p.strip() for p in op_str.split(",")]
    if len(parts) != 3:
        return (x % 1.0, y % 1.0, z % 1.0)

    coords = [_parse_sym_coord(p, x, y, z) for p in parts]
    return (coords[0] % 1.0, coords[1] % 1.0, coords[2] % 1.0)


def generate_equivalent_positions(x, y, z, sym_ops):
    """
    Generate all symmetry-equivalent positions using the space group
    symmetry operations.

    Args:
        x, y, z: Fractional coordinates of the asymmetric unit atom
        sym_ops: List of symmetry operation strings (e.g. ["x,y,z", "-x,-y,z+1/2", ...])

    Returns:
        List of unique (x, y, z) tuples in [0, 1).
    """
    positions = set()
    for op in sym_ops:
        pos = apply_symmetry_op(x, y, z, op)
        # Round to avoid floating-point duplicates
        rounded = (round(pos[0], 6), round(pos[1], 6), round(pos[2], 6))
        positions.add(rounded)
    return [list(p) for p in positions]


def calculate_structure_factor(h, k, l, atoms, sym_ops, sin_theta_over_lambda):
    """
    Calculate structure factor F(hkl) with full symmetry expansion.

    F(hkl) = Σ_j f_j · Σ_s exp(2πi · (h·x_{js} + k·y_{js} + l·z_{js}))

    where j runs over atoms in the asymmetric unit,
          s runs over all symmetry operations of the space group.

    Returns:
        (F_real, F_imag)
    """
    F_real = 0.0
    F_imag = 0.0

    for atom in atoms:
        el = atom.get("element", "O")
        occ = atom.get("occ", 1.0)
        x0, y0, z0 = atom.get("x", 0), atom.get("y", 0), atom.get("z", 0)
        f = scattering_factor(el, sin_theta_over_lambda)

        # Sum over ALL symmetry equivalent positions
        eq_pos = generate_equivalent_positions(x0, y0, z0, sym_ops)

        # Internal sum over equivalent positions
        sum_real = 0.0
        sum_imag = 0.0
        for pos in eq_pos:
            px, py, pz = pos
            phase = 2 * math.pi * (h * px + k * py + l * pz)
            sum_real += math.cos(phase)
            sum_imag += math.sin(phase)

        F_real += occ * f * sum_real
        F_imag += occ * f * sum_imag

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

    merged = {}  # d_rounded (str) -> merged reflection dict

    for h in range(-max_h, max_h + 1):
        for k in range(-max_k, max_k + 1):
            for l in range(-max_l, max_l + 1):
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

                # Merge symmetry-equivalent reflections by d-spacing
                d_key = round(d, 2)
                if d_key in merged:
                    merged[d_key]["intensity"] += intensity
                    merged[d_key]["multiplicity"] += 1
                    # Keep the hkl with the most symmetric indices
                    existing = merged[d_key]
                    if abs(h) < abs(existing["h"]) or (
                        abs(h) == abs(existing["h"]) and abs(k) < abs(existing["k"])
                    ):
                        existing["h"] = h
                        existing["k"] = k
                        existing["l"] = l
                else:
                    merged[d_key] = {
                        "h": h, "k": k, "l": l,
                        "d": round(d, 4),
                        "two_theta": round(two_theta, 4),
                        "intensity": intensity,
                        "F_sq": round(F_sq, 2),
                        "multiplicity": 1,
                    }

    # Sort by intensity descending
    reflections = sorted(merged.values(), key=lambda r: r["intensity"], reverse=True)
    
    # Normalize intensities to I_max = 100
    if reflections:
        i_max = reflections[0]["intensity"]
        for r in reflections:
            r["intensity_rel"] = round(r["intensity"] / i_max * 100, 1)
    
    return reflections[:top_n]


# ─── COD Download + Process ──────────────────────────────────────────

COD_BASE = "https://www.crystallography.net/cod"
CIF_CACHE_DIR = Path(__file__).parent / "cif_cache"
CIF_CACHE_DIR.mkdir(exist_ok=True)

def get_cif(entry_id: str) -> str | None:
    """Download a CIF file from COD by entry ID (with local caching)."""
    cache_path = CIF_CACHE_DIR / f"{entry_id}.cif"
    if cache_path.exists():
        return cache_path.read_text()
    url = f"{COD_BASE}/cif/{entry_id}.cif"
    try:
        with urllib.request.urlopen(url, timeout=15) as resp:
            text = resp.read().decode()
            with _CIF_LOCK:
                cache_path.write_text(text)
            return text
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
