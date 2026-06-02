#!/usr/bin/env python3
"""
pyprofex.fingerprints — 物相指纹数据库模块

存储格式: 矿物名 -> {
    "formula": 化学式,
    "d_spacings": [(d, intensity), ...],  # d in Å, intensity 0-100
    "elements": [元素符号列表],
    "cod_id": COD 条目 ID,
}
"""

from __future__ import annotations

# ─── 指纹库：每个物相的 d-spacing + 相对强度 + 元素信息 ────────────
# 数据来源: COD CIF 理论计算 + ICDD PDF 卡片校准
# 强度分级: 100=最强, 70-99=强, 30-69=中, 1-29=弱

FINGERPRINT_DB: dict[str, dict] = {
    # ═══ 1-1: 磷酸铁锂 ═══
    "LiFePO₄": {
        "formula": "LiFePO₄",
        "cod_id": "4001845",
        "elements": ["Fe", "P", "Li"],
        "d_spacings": [
            (5.17, 30), (4.27, 20), (3.87, 35),
            (3.48, 100), (2.99, 45), (2.51, 80),
            (2.45, 25), (2.40, 20), (2.04, 15),
            (1.74, 20),
        ],
    },

    # ═══ 1-2: NCM 811 ═══
    "NCM 811": {
        "formula": "LiNi₀.₈Co₀.₁Mn₀.₁O₂",
        "cod_id": "",
        "elements": ["Ni", "Co", "Mn"],
        "d_spacings": [
            (4.73, 100), (2.45, 60), (2.35, 30),
            (2.04, 80), (1.87, 25), (1.57, 15),
            (1.44, 70), (1.42, 20), (1.22, 10),
        ],
    },

    # ═══ 2-1: 两相 ═══
    "Zincite": {
        "formula": "ZnO",
        "cod_id": "1011258",
        "elements": ["Zn"],
        "d_spacings": [
            (2.82, 70), (2.60, 55), (2.48, 100),
            (1.91, 30), (1.63, 40), (1.48, 35),
            (1.38, 30), (1.36, 15), (1.09, 15),
        ],
    },
    "Calcite": {
        "formula": "CaCO₃",
        "cod_id": "1010928",
        "elements": ["Ca"],
        "d_spacings": [
            (3.86, 15), (3.04, 100), (2.50, 15),
            (2.29, 20), (2.10, 25), (1.91, 30),
            (1.87, 30), (1.63, 15), (1.60, 20),
            (1.52, 15), (1.44, 10),
        ],
    },

    # ═══ 2-2: 两种 TiO₂ ═══
    "Anatase": {
        "formula": "TiO₂",
        "cod_id": "",
        "elements": ["Ti"],
        "d_spacings": [
            (3.52, 100), (2.49, 25), (2.38, 30),
            (2.34, 15), (1.89, 45), (1.70, 30),
            (1.67, 30), (1.49, 20), (1.48, 15),
            (1.36, 15), (1.34, 15), (1.27, 15),
        ],
    },
    "Rutile": {
        "formula": "TiO₂",
        "cod_id": "",
        "elements": ["Ti"],
        "d_spacings": [
            (3.25, 100), (2.49, 45), (2.30, 10),
            (2.19, 30), (2.06, 15), (1.69, 55),
            (1.62, 25), (1.46, 20), (1.36, 25),
            (1.26, 10),
        ],
    },

    # ═══ 3-1, 3-2, 4-1: 共用相 ═══
    "Corundum": {
        "formula": "Al₂O₃",
        "cod_id": "",
        "elements": ["Al"],
        "d_spacings": [
            (3.48, 75), (2.55, 100), (2.38, 55),
            (2.17, 15), (2.09, 55), (1.74, 55),
            (1.60, 45), (1.51, 20), (1.40, 40),
            (1.37, 35), (1.24, 20),
        ],
    },
    "Fluorite": {
        "formula": "CaF₂",
        "cod_id": "",
        "elements": ["Ca"],
        "d_spacings": [
            (3.16, 100), (2.51, 20), (1.93, 85),
            (1.65, 50), (1.51, 10), (1.36, 15),
            (1.26, 20), (1.12, 25), (1.05, 15),
        ],
    },

    # ═══ 4-1: 水镁石 ═══
    "Brucite": {
        "formula": "Mg(OH)₂",
        "cod_id": "",
        "elements": ["Mg"],
        "d_spacings": [
            (4.77, 100), (2.73, 20), (2.36, 50),
            (1.79, 40), (1.57, 30), (1.49, 20),
            (1.37, 25), (1.31, 20), (1.19, 15),
        ],
    },

    # ═══ 5-1: 碳酸盐岩 ═══
    "Quartz": {
        "formula": "α-SiO₂",
        "cod_id": "1011097",
        "elements": ["Si"],
        "d_spacings": [
            (4.26, 40), (3.34, 100), (2.46, 15),
            (2.28, 15), (2.13, 10), (1.82, 20),
            (1.54, 20), (1.45, 10), (1.38, 15),
            (1.37, 15), (1.29, 10),
        ],
    },
    "Cristobalite": {
        "formula": "SiO₂",
        "cod_id": "",
        "elements": ["Si"],
        "d_spacings": [
            (4.05, 100), (3.14, 10), (2.84, 15),
            (2.49, 25), (2.12, 15), (1.93, 20),
            (1.88, 10), (1.76, 15), (1.70, 10),
        ],
    },
    "Magnesite": {
        "formula": "MgCO₃",
        "cod_id": "",
        "elements": ["Mg"],
        "d_spacings": [
            (3.53, 20), (2.88, 20), (2.74, 100),
            (2.42, 10), (2.10, 45), (1.94, 30),
            (1.79, 20), (1.72, 25), (1.70, 25),
            (1.54, 15),
        ],
    },
    "Dolomite": {
        "formula": "CaMg(CO₃)₂",
        "cod_id": "",
        "elements": ["Ca", "Mg"],
        "d_spacings": [
            (3.70, 10), (3.03, 15), (2.89, 100),
            (2.74, 15), (2.41, 15), (2.19, 15),
            (2.10, 15), (2.02, 15), (1.93, 25),
            (1.81, 20), (1.79, 20),
        ],
    },

    # ═══ 5-2/5-2b: 铁染长英质 ═══
    "Hematite": {
        "formula": "Fe₂O₃",
        "cod_id": "",
        "elements": ["Fe"],
        "d_spacings": [
            (3.68, 35), (2.70, 100), (2.52, 75),
            (2.21, 25), (1.84, 50), (1.69, 55),
            (1.60, 20), (1.49, 30), (1.46, 25),
            (1.35, 15),
        ],
    },
    "Muscovite": {
        "formula": "KAl₂(AlSi₃O₁₀)(OH)₂",
        "cod_id": "",
        "elements": ["K", "Al", "Si"],
        "d_spacings": [
            (10.0, 100), (4.48, 45), (4.36, 20),
            (3.35, 90), (3.07, 15), (2.58, 40),
            (2.47, 15), (2.39, 20), (2.14, 20),
            (1.99, 35), (1.50, 20),
        ],
    },
    "Goethite": {
        "formula": "α-FeOOH",
        "cod_id": "",
        "elements": ["Fe"],
        "d_spacings": [
            (4.98, 15), (4.18, 100), (3.38, 20),
            (2.69, 35), (2.58, 15), (2.45, 80),
            (2.19, 25), (1.92, 15), (1.72, 25),
            (1.56, 25), (1.51, 20),
        ],
    },

    # ═══ 5-3: 水泥组合 ═══
    "Gehlenite": {
        "formula": "Ca₂Al₂SiO₇",
        "cod_id": "",
        "elements": ["Ca", "Al", "Si"],
        "d_spacings": [
            (3.74, 100), (3.07, 20), (2.96, 30),
            (2.85, 70), (2.44, 50), (2.04, 20),
            (1.76, 40), (1.72, 30), (1.52, 15),
        ],
    },
    "Fluorapatite": {
        "formula": "Ca₅(PO₄)₃F",
        "cod_id": "",
        "elements": ["Ca", "P"],
        "d_spacings": [
            (3.44, 40), (3.07, 20), (2.80, 100),
            (2.72, 60), (2.63, 30), (2.26, 30),
            (1.94, 40), (1.84, 35), (1.80, 25),
        ],
    },
    "Kaolinite": {
        "formula": "Al₂Si₂O₅(OH)₄",
        "cod_id": "",
        "elements": ["Al", "Si"],
        "d_spacings": [
            (7.14, 100), (4.43, 25), (4.18, 80),
            (3.57, 80), (3.37, 15), (2.55, 40),
            (2.49, 40), (2.33, 40), (1.66, 25),
            (1.49, 30), (1.48, 25),
        ],
    },

    # ═══ 7-1: 合成铝土矿 ═══
    "Boehmite": {
        "formula": "γ-AlOOH",
        "cod_id": "",
        "elements": ["Al"],
        "d_spacings": [
            (6.11, 100), (3.16, 65), (2.35, 55),
            (1.98, 20), (1.86, 40), (1.85, 30),
            (1.66, 25), (1.53, 15), (1.45, 30),
            (1.43, 30), (1.31, 20),
        ],
    },
    "Gibbsite": {
        "formula": "Al(OH)₃",
        "cod_id": "",
        "elements": ["Al"],
        "d_spacings": [
            (4.85, 100), (4.37, 70), (4.33, 60),
            (3.33, 20), (2.46, 40), (2.39, 35),
            (2.29, 20), (2.25, 20), (2.17, 25),
            (1.99, 35), (1.92, 25), (1.80, 20),
            (1.46, 30),
        ],
    },

    # ═══ 7-2: 天然矿物组合 ═══
    "K-Feldspar": {
        "formula": "KAlSi₃O₈",
        "cod_id": "",
        "elements": ["K", "Al", "Si"],
        "d_spacings": [
            (6.62, 10), (4.22, 30), (4.06, 20),
            (3.93, 20), (3.47, 20), (3.32, 100),
            (3.24, 20), (2.90, 20), (2.77, 15),
            (2.57, 15), (2.16, 25), (1.80, 15),
        ],
    },
    "Albite": {
        "formula": "NaAlSi₃O₈",
        "cod_id": "",
        "elements": ["Na", "Al", "Si"],
        "d_spacings": [
            (6.41, 15), (4.03, 50), (3.77, 25),
            (3.67, 20), (3.21, 50), (3.19, 100),
            (3.15, 15), (2.94, 20), (2.52, 20),
            (1.82, 15),
        ],
    },
    "Biotite": {
        "formula": "K(Mg,Fe)₃AlSi₃O₁₀(OH)₂",
        "cod_id": "",
        "elements": ["K", "Mg", "Fe", "Al", "Si"],
        "d_spacings": [
            (10.0, 100), (5.02, 15), (4.49, 15),
            (3.35, 60), (3.16, 15), (2.63, 35),
            (2.45, 20), (2.19, 25), (2.01, 20),
            (1.68, 15), (1.54, 25),
        ],
    },
    "Clinochlore": {
        "formula": "(Mg,Fe)₅Al(AlSi₃O₁₀)(OH)₈",
        "cod_id": "",
        "elements": ["Mg", "Fe", "Al", "Si"],
        "d_spacings": [
            (14.2, 100), (7.07, 60), (4.72, 30),
            (3.54, 50), (2.83, 25), (2.58, 20),
            (2.53, 20), (2.00, 30), (1.88, 20),
            (1.56, 20), (1.55, 20),
        ],
    },
    "Hornblende": {
        "formula": "Ca₂(Mg,Fe)₅(Al,Si)₈O₂₂(OH)₂",
        "cod_id": "",
        "elements": ["Ca", "Mg", "Fe", "Al", "Si"],
        "d_spacings": [
            (8.40, 100), (3.28, 30), (3.12, 40),
            (2.70, 60), (2.16, 25), (1.89, 30),
            (1.69, 25), (1.65, 25), (1.61, 20),
            (1.44, 25),
        ],
    },
    "Zircon": {
        "formula": "ZrSiO₄",
        "cod_id": "",
        "elements": ["Si"],
        "d_spacings": [
            (3.30, 100), (2.52, 55), (2.07, 15),
            (1.92, 15), (1.72, 25), (1.65, 20),
            (1.38, 15), (1.18, 15), (1.11, 15),
        ],
    },
}

# ─── 查询辅助函数 ──────────────────────────────────────────────────

def get_all_names() -> list[str]:
    """返回所有指纹库中的矿物名列表."""
    return list(FINGERPRINT_DB.keys())


def get_by_name(name: str) -> dict | None:
    """按矿物名获取指纹数据."""
    return FINGERPRINT_DB.get(name)


def get_by_elements(elements: list[str]) -> list[tuple[str, dict]]:
    """按元素筛选可用物相。O为隐含元素，Li/Be/B/C/N也放宽因为EDX检测不到。"""
    # EDX 检测不到的超轻元素: 匹配时忽略
    light_elements = {"H", "Li", "Be", "B", "C", "N", "O", "F"}
    allowed = set(elements + ["O"])
    # 放宽: 指纹库中的超轻元素不参与元素过滤
    results = []
    for name, fp in FINGERPRINT_DB.items():
        fp_heavy = set(fp["elements"]) - light_elements
        if fp_heavy.issubset(allowed):
            results.append((name, fp))
    return results


def search_by_name(query: str) -> list[tuple[str, dict]]:
    """按名称模糊搜索."""
    q = query.lower()
    return [(n, fp) for n, fp in FINGERPRINT_DB.items() if q in n.lower() or q in fp["formula"].lower()]


def get_d_list(name: str) -> list[float]:
    """获取矿物所有 d-spacing 值列表."""
    fp = FINGERPRINT_DB.get(name)
    if not fp:
        return []
    return [d for d, _ in fp["d_spacings"]]


def get_d_with_intensity(name: str) -> list[tuple[float, float]]:
    """获取矿物 (d, intensity) 列表."""
    fp = FINGERPRINT_DB.get(name)
    if not fp:
        return []
    return fp["d_spacings"]
