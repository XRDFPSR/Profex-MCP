#!/usr/bin/env python3
"""
pyprofex.search_match — Search-Match 引擎
整合三个指纹源:
1. fingerprints.py (手动验证, 27 矿物)
2. fingerprints_auto.py (CIF 自建, ~2200 矿物, 后台继续)
3. fingerprints_powcod.json (Qualx2 POWCOD, 13944 矿物)

用法:
  from search_match import independent_scoring, suggest_elements
  result = independent_scoring(obs_d, elements=["Fe","O"])
"""
from __future__ import annotations
import json
import math
import os
import re
import sys
from pathlib import Path

# 金属元素集合 — 用于过滤有机物
_METAL_ELEMENTS = frozenset({
    "Li","Be","Na","Mg","Al","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co",
    "Ni","Cu","Zn","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag",
    "Cd","In","Sn","Sb","Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu",
    "Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os","Ir",
    "Pt","Au","Hg","Tl","Pb","Bi","Po","At","Fr","Ra","Ac","Th","Pa","U",
    "Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr"})

# 有机物标记元素
_ORGANIC_KEYWORDS = frozenset({"C", "H", "N", "S"})

# ─── 指纹数据库加载 ────────────────────────────────────────────────

_FINGERPRINT_DB: dict | None = None

def _load_unified_db() -> dict:
    """从预编译 pickle 加载统一指纹库（最快路径）。
    兼容 PyInstaller 打包后的路径查找。"""
    import pickle
    # PyInstaller 打包后 __file__ 可能在临时目录
    try:
        base = Path(sys._MEIPASS)
    except AttributeError:
        base = Path(__file__).parent
    
    pkl_path = base / "pyprofex" / "fingerprints_unified.pkl"
    if not pkl_path.exists():
        # Fallback: same directory
        pkl_path = base / "fingerprints_unified.pkl"
    if pkl_path.exists():
        return pickle.loads(pkl_path.read_bytes())
    return {}

def _load_manual_db():
    """加载手动指纹库 fingerprints.py"""
    try:
        from fingerprints import FINGERPRINT_DB
        return {"manual_" + k: v for k, v in FINGERPRINT_DB.items()}
    except ImportError:
        return {}

def _load_auto_db():
    """加载自动指纹库 fingerprints_auto.py"""
    try:
        from fingerprints_auto import FINGERPRINT_DB_AUTO
        return FINGERPRINT_DB_AUTO
    except ImportError:
        return {}

def _load_powcod_db():
    """加载 POWCOD 指纹库 fingerprints_powcod.json"""
    db_path = Path(__file__).parent / "fingerprints_powcod.json"
    if not db_path.exists():
        return {}
    with open(db_path) as f:
        return json.load(f)

def _normalize_fp(entry: dict, key: str = "") -> dict:
    """
    将不同格式的指纹统一为标准格式:
    {
      "name": str,
      "formula": str,
      "elements": [str, ...],
      "d_spacings": [(d, intensity), ...],  # 降序排列
    }
    """
    name = entry.get("name") or entry.get("mineral") or entry.get("n") or key
    formula = entry.get("formula") or entry.get("chemical_formula") or entry.get("f") or ""
    
    # Extract peaks: handle multiple formats
    peaks = entry.get("peaks") or entry.get("d_spacings") or entry.get("p", [])
    if isinstance(peaks, list) and len(peaks) > 0:
        # peaks = [(d, I), ...] or [[d, I], ...] or [{"d":..., "intensity":...}, ...]
        first = peaks[0]
        if isinstance(first, (list, tuple)):
            # [(d, I), ...] or [[d, I], ...]
            d_spacings = [(round(p[0], 4), round(p[1], 1)) for p in peaks if isinstance(p, (list, tuple)) and len(p) >= 2]
        elif isinstance(first, dict):
            d_spacings = [(round(p.get("d", 0), 4), round(p.get("intensity", 100), 1)) for p in peaks]
        else:
            d_spacings = []
    else:
        d_spacings = []
    
    # Also handle d_spacings key from old format (each = (d, I) tuple)
    if not d_spacings and isinstance(peaks, dict):
        d_list = peaks.get("d_spacings", [])
        i_list = peaks.get("intensities", [])
        if d_list and i_list:
            d_spacings = [(round(d, 4), round(i, 1)) for d, i in zip(d_list, i_list) if 0.5 <= d <= 100]
    
    if not d_spacings:
        return {}
    
    # Sort by intensity descending
    d_spacings.sort(key=lambda x: x[1], reverse=True)
    
    # Extract elements from formula
    elements = entry.get("elements", [])
    if not elements and formula:
        import re
        elements = list(set(re.findall(r'[A-Z][a-z]?', formula)))
        elements = [e for e in elements if e[0].isalpha() and e not in ('X', 'R')]
    
    return {
        "name": name,
        "formula": formula,
        "elements": elements,
        "d_spacings": d_spacings,
        "_source": entry.get("source") or entry.get("_source", "unknown"),
    }


def get_db() -> dict:
    """加载并合并所有指纹源。惰性加载，只执行一次。
    优先使用预编译 pickle（最快），无则从各源动态构建。"""
    global _FINGERPRINT_DB
    if _FINGERPRINT_DB is not None:
        return _FINGERPRINT_DB
    
    # Try fastest path first: pre-built pickle
    merged = _load_unified_db()
    if merged:
        _FINGERPRINT_DB = merged
        return merged
    
    # Fallback: dynamic build from sources
    merged = {}
    
    # 1. 手动验证库 (最高优先级)
    manual = _load_manual_db()
    for key, entry in manual.items():
        norm = _normalize_fp(entry, key)
        if norm and norm["d_spacings"]:
            norm["_source"] = "manual"
            merged[key] = norm
    
    # 2. 自建 CIF 库
    auto = _load_auto_db()
    for key, entry in auto.items():
        norm = _normalize_fp(entry, key)
        if norm and norm["d_spacings"]:
            norm["_source"] = "auto_cif"
            nk = norm["name"].lower()
            if not any(ex["name"].lower() == nk for ex in merged.values()):
                merged[key] = norm
    
    # 3. POWCOD 库 — 只保留无机物（排除纯有机物）
    powcod = _load_powcod_db()
    
    for key, entry in powcod.items():
        norm = _normalize_fp(entry, key)
        if norm and norm["d_spacings"]:
            norm["_source"] = "powcod"
            # Skip pure organics: has C/H/N but no metal
            els = set(norm.get("elements", []))
            if (els & _ORGANIC_KEYWORDS) and not (els & _METAL_ELEMENTS):
                continue
            nk = norm["name"].lower()
            if not any(ex["name"].lower() == nk for ex in merged.values()):
                # Use mineral name as key
                safe_key = norm["name"].lower().replace(" ", "_").replace("-", "_")
                safe_key = "".join(c for c in safe_key if c.isalnum() or c == "_")
                if not safe_key:
                    safe_key = key
                if safe_key in merged:
                    safe_key = f"{safe_key}_{len(merged)}"
                merged[safe_key] = norm
    
    _FINGERPRINT_DB = merged
    return merged


def get_by_elements(elements: list[str]) -> list[tuple[str, dict]]:
    """根据元素筛选候选指纹。返回 [(name, fp), ...]"""
    db = get_db()
    if not elements:
        return list(db.items())
    
    elements_set = set(elements)
    candidates = []
    for key, fp in db.items():
        fp_elements = set(fp.get("elements", []))
        if fp_elements & elements_set:  # 至少一个共同元素
            candidates.append((key, fp))
    
    return candidates


def suggest_elements(obs_d: list[float], top_n: int = 5, tol: float = 0.025) -> list[str]:
    """根据实验 d-spacing 自动推断可能元素。"""
    db = get_db()
    scores = {}
    for key, fp in db.items():
        match_count = 0
        for ref_d, _ in fp["d_spacings"]:
            for od in obs_d:
                if abs(od - ref_d) / ref_d <= tol:
                    match_count += 1
                    break
        if match_count >= 3:
            for el in fp.get("elements", []):
                scores[el] = scores.get(el, 0) + match_count
    return sorted(scores, key=scores.get, reverse=True)[:top_n]


MATCH_TOLERANCE = 0.025

def match_d(obs_d: float, ref_d: float, tol: float = MATCH_TOLERANCE) -> bool:
    if ref_d <= 0: return False
    return abs(obs_d - ref_d) / ref_d <= tol

def weight(i: float) -> float:
    return 3.0 if i >= 70 else (2.0 if i >= 30 else 1.0)

def score_single_phase(obs_d, ref_di, tol=MATCH_TOLERANCE):
    matched_pairs = []; used_obs = set(); used_ref = set()
    sorted_ref = sorted(enumerate(ref_di), key=lambda x: x[1][1], reverse=True)
    for ri, (ref_d, ref_i) in sorted_ref:
        best_oi = None; best_diff = float("inf")
        for oi, od in enumerate(obs_d):
            if oi in used_obs: continue
            diff = abs(od - ref_d) / ref_d
            if diff <= tol and diff < best_diff:
                best_diff = diff; best_oi = oi
        if best_oi is not None:
            matched_pairs.append((obs_d[best_oi], ref_d, ref_i, weight(ref_i)))
            used_obs.add(best_oi); used_ref.add(ri)
    total_w = sum(weight(i) for _, i in ref_di)
    matched_w = sum(w for _, _, _, w in matched_pairs)
    i_score = matched_w / total_w if total_w > 0 else 0
    strong_matched = sum(1 for ri,(_,ri2) in enumerate(ref_di) if ri2>=70 and ri in used_ref)
    strong_missed = sum(1 for ri,(_,ri2) in enumerate(ref_di) if ri2>=70 and ri not in used_ref)
    return {
        "matches": len(matched_pairs), "matched_pairs": matched_pairs,
        "unmatched_obs": [d for i,d in enumerate(obs_d) if i not in used_obs],
        "intensity_score": round(i_score, 3),
        "preferred_orientation": (strong_missed > 0 and strong_matched >= 1),
        "strong_matched": strong_matched, "strong_missed": strong_missed,
    }

def fom_score(mr, total_obs):
    denom = 1 + len(mr["unmatched_obs"])
    return round(mr["matches"] * mr["intensity_score"] / denom, 3) if denom > 0 else 0.0

def independent_score_all(obs_d, elements, n_expected=0, tol=MATCH_TOLERANCE, min_peaks=2):
    candidates = get_by_elements(elements)
    if not candidates:
        return {"phases":[],"unmatched_peaks":obs_d,"total_phases_found":0,"coverage":0}
    scored = []
    for name, fp in candidates:
        result = score_single_phase(obs_d, fp["d_spacings"], tol)
        if result["matches"] < min_peaks: continue
        fom_v = fom_score(result, len(obs_d))
        elem_cov = len(set(fp["elements"])&set(elements))/max(len(set(elements)),1)
        scored.append((fom_v*10+elem_cov*3, name, fp, result))
    scored.sort(key=lambda x: x[0], reverse=True)
    n_take = n_expected if n_expected > 0 else len(scored)
    covered_d = set(); phases_out = []
    for total_score, name, fp, result in scored[:n_take]:
        phases_out.append({
            "name": name, "formula": fp["formula"],
            "matches": result["matches"],
            "matched_d": [round(p[0],4) for p in result["matched_pairs"]],
            "intensity_coverage": result["intensity_score"],
            "fom": fom_score(result, len(obs_d)),
            "element_coverage": round(len(set(fp["elements"])&set(elements))/max(len(set(elements)),1),2),
            "total_score": round(total_score,1),
            "preferred_orientation": result["preferred_orientation"],
        })
        for p in result["matched_pairs"]: covered_d.add(round(p[0],4))
    unmatched = [d for d in obs_d if d not in covered_d]
    return {
        "phases": phases_out, "unmatched_peaks": [round(d,4) for d in unmatched],
        "total_phases_found": len(phases_out),
        "coverage": round(len(covered_d)/len(obs_d),3) if obs_d else 0,
        "total_obs_peaks": len(obs_d),
    }

def subtract_phase(obs_d, ref_di, tol=MATCH_TOLERANCE):
    """从观察峰列表中扣除已匹配的峰。
    只扣除强度排名前 60% 的理论峰（避免过度扣除共享峰）。"""
    matched = set()
    # Sort by intensity descending
    sorted_ref = sorted(enumerate(ref_di), key=lambda x: x[1][1], reverse=True)
    top_count = max(3, int(len(sorted_ref) * 0.6))
    for ri, (ref_d, _) in sorted_ref[:top_count]:
        for oi, od in enumerate(obs_d):
            if oi not in matched and match_d(od, ref_d, tol):
                matched.add(oi)
                break
    return [d for i, d in enumerate(obs_d) if i not in matched]


def iterative_search_match(
    obs_d, elements, n_expected=0, tol=MATCH_TOLERANCE,
    min_peaks=3, min_fom=0.4
):
    """
    改进版多相迭代扣除 Search-Match。

    策略:
    1. 预筛 — 对所有候选评分，取 top_N 候选
    2. 独占匹配评分 — 每个矿物的匹配峰中，有多少不被其他 top 候选匹配
       （独占匹配越多，越可能是真实相，而非宽匹配型干扰相）
    3. 选独占匹配最优相 → 扣除 → 迭代
    4. 避免重复选同一矿物名的不同 CID 条目
    """
    candidates = get_by_elements(elements)
    if not candidates:
        return {"phases": [], "unmatched_peaks": obs_d,
                "total_phases_found": 0, "coverage": 0, "total_obs_peaks": len(obs_d)}
    
    total_obs = len(obs_d)
    max_iter = n_expected if n_expected > 0 else min(8, len(obs_d) // 2)
    remaining = list(obs_d)
    identified: list[dict] = []
    selected_names: set[str] = set()
    
    for iteration in range(max_iter):
        if len(remaining) < min_peaks:
            break
        
        # ── Step 1: 初筛 — 对所有候选评分 ──
        all_scores = []
        fp_prefetch = []
        for name, fp in candidates:
            base_name = fp["name"].lower().strip()
            if base_name in selected_names:
                continue
            result = score_single_phase(remaining, fp["d_spacings"], tol)
            if result["matches"] < min_peaks:
                continue
            fom_v = fom_score(result, len(remaining))
            if fom_v < min_fom:
                continue
            all_scores.append((fom_v, name, fp, result))
        
        if len(all_scores) < 1:
            break
        
        # Sort by FOM, take top N for competitive analysis
        all_scores.sort(key=lambda x: x[0], reverse=True)
        top_n = min(500, len(all_scores)) if len(obs_d) > 8 else min(300, len(all_scores))
        top_field = all_scores[:top_n]
        
        # ── Step 2: 稀有度加权独占评分 ──
        # For each obs peak, count how many of top_N candidates can match it.
        # A peak matched by FEW candidates is "rare" — high weight.
        # A peak matched by MANY candidates is "common" — low weight.
        # The best candidate is the one matching many rare peaks.
        
        # Count candidates per peak
        peak_match_count = [0] * len(remaining)
        for idx, (fom_v, name, fp, result) in enumerate(top_field):
            matched_obs_indices = set()
            for od, _, _, _ in result["matched_pairs"]:
                for oi, ood in enumerate(remaining):
                    if oi < len(remaining) and abs(ood - od) / max(od, 0.001) <= tol:
                        matched_obs_indices.add(oi)
                        break
            for mi in matched_obs_indices:
                peak_match_count[mi] += 1
        
        # Rare peak: matched by <= 20% of top_field
        rare_threshold = max(1, int(top_n * 0.15))
        rare_peak_indices = {
            i for i, c in enumerate(peak_match_count)
            if 0 < c <= rare_threshold
        }
        
        # Score each top candidate by how many rare peaks it covers
        for idx, (fom_v, name, fp, result) in enumerate(top_field):
            matched_obs_indices = set()
            for od, _, _, _ in result["matched_pairs"]:
                for oi, ood in enumerate(remaining):
                    if abs(ood - od) / max(od, 0.001) <= tol:
                        matched_obs_indices.add(oi)
                        break
            
            n_matched = len(matched_obs_indices)
            n_rare = sum(1 for mi in matched_obs_indices if mi in rare_peak_indices)
            
            # Final score = FOM * (1 + rare_fraction)
            rare_fraction = n_rare / max(n_matched, 1)
            rare_score = fom_v * (1 + rare_fraction * 3)
            
            # Penalty: if fingerprint has way more peaks than obs, it's too generic
            n_fp_peaks = len(fp["d_spacings"])
            penalty = 0
            ratio = n_fp_peaks / max(len(remaining), 1)
            if ratio > 5:
                penalty = 3.0
            elif ratio > 3:
                penalty = ratio * 0.12
            elif ratio > 2:
                penalty = ratio * 0.06
            rare_score -= penalty
            
            # Element penalty — penalize extra elements not in query
            fp_els = set(fp.get("elements", []))
            query_els = set(elements) if elements else set()
            if query_els:
                extra = len(fp_els - query_els)
                rare_score -= extra * 0.15
            
            top_field[idx] = (rare_score, name, fp, result, rare_score)
        
        # ── Step 3: 选最优 ──
        top_field.sort(key=lambda x: x[0], reverse=True)
        best_score, best_name, best_fp, best_result, _ = top_field[0]
        
        # Record
        identified.append({
            "name": best_fp["name"],
            "formula": best_fp["formula"],
            "matches": best_result["matches"],
            "matched_d": [round(p[0], 4) for p in best_result["matched_pairs"]],
            "intensity_coverage": best_result["intensity_score"],
            "fom": fom_score(best_result, len(remaining)),
            "element_coverage": round(
                len(set(best_fp.get("elements", [])) & set(elements)) / max(len(set(elements)), 1), 2
            ) if elements else 1.0,
            "total_score": round(best_score, 2),
            "preferred_orientation": best_result["preferred_orientation"],
        })
        selected_names.add(best_fp["name"].lower().strip())
        
        # Subtract
        prev_len = len(remaining)
        remaining = subtract_phase(remaining, best_fp["d_spacings"], tol)
        
        if n_expected > 0 and len(identified) >= n_expected:
            break
    
    return {
        "phases": identified,
        "unmatched_peaks": [round(d, 4) for d in remaining],
        "total_phases_found": len(identified),
        "coverage": round((total_obs - len(remaining)) / total_obs, 3) if total_obs else 0,
        "total_obs_peaks": total_obs,
    }

# ─── S2.2: 自适应容差版本 ─────────────────────────────────

def score_single_phase_adaptive(obs_d, ref_di, base_tol=0.025):
    """自适应容台版的独立评分."""
    try:
        from peaks import adaptive_tolerance, match_d_adaptive
    except ImportError:
        def adaptive_tolerance(d, bt=0.025):
            if d < 2.0: return bt
            elif d < 4.0: return bt*1.3
            else: return bt*1.6
        def match_d_adaptive(obs_d, ref_d, bt=0.025):
            if ref_d<=0: return False
            return abs(obs_d-ref_d)/ref_d <= adaptive_tolerance(ref_d, bt)
    
    matched_pairs = []; used_obs = set(); used_ref = set()
    sorted_ref = sorted(enumerate(ref_di), key=lambda x: x[1][1], reverse=True)
    for ri, (ref_d, ref_i) in sorted_ref:
        best_oi = None; best_diff = float("inf")
        for oi, od in enumerate(obs_d):
            if oi in used_obs: continue
            diff = abs(od-ref_d)/ref_d
            tol = adaptive_tolerance(ref_d, base_tol)
            if diff <= tol and diff < best_diff:
                best_diff = diff; best_oi = oi
        if best_oi is not None:
            w = 3.0 if ref_i>=70 else (2.0 if ref_i>=30 else 1.0)
            matched_pairs.append((obs_d[best_oi], ref_d, ref_i, w))
            used_obs.add(best_oi); used_ref.add(ri)
    total_w = sum(3.0 if i>=70 else (2.0 if i>=30 else 1.0) for _,i in ref_di)
    matched_w = sum(w for _,_,_,w in matched_pairs)
    i_score = matched_w/total_w if total_w>0 else 0
    strong_m = sum(1 for ri,(_,ri2) in enumerate(ref_di) if ri2>=70 and ri in used_ref)
    strong_miss = sum(1 for ri,(_,ri2) in enumerate(ref_di) if ri2>=70 and ri not in used_ref)
    unmatched = [d for i,d in enumerate(obs_d) if i not in used_obs]
    return {
        "matches": len(matched_pairs), "matched_pairs": matched_pairs,
        "unmatched_obs": unmatched,
        "intensity_score": round(i_score, 3),
        "preferred_orientation": (strong_miss>0 and strong_m>=1),
        "strong_matched": strong_m, "strong_missed": strong_miss,
    }

def independent_score_all_adaptive(obs_d, elements, n_expected=0, base_tol=0.025, min_peaks=2):
    """自适应容台版独立评分."""
    from fingerprints import get_by_elements
    candidates = get_by_elements(elements)
    if not candidates:
        return {"phases":[],"unmatched_peaks":obs_d,"total_phases_found":0,"coverage":0}
    scored = []
    for name, fp in candidates:
        result = score_single_phase_adaptive(obs_d, fp["d_spacings"], base_tol)
        if result["matches"] < min_peaks: continue
        fom_v = fom_score(result, len(obs_d))  # reuse standard fom
        elem_cov = len(set(fp["elements"])&set(elements))/max(len(set(elements)),1)
        scored.append((fom_v*10+elem_cov*3, name, fp, result))
    scored.sort(key=lambda x: x[0], reverse=True)
    n_take = n_expected if n_expected>0 else len(scored)
    covered_d = set(); phases_out = []
    for total_score, name, fp, result in scored[:n_take]:
        phases_out.append({
            "name":name,"formula":fp["formula"],"matches":result["matches"],
            "matched_d":[round(p[0],4) for p in result["matched_pairs"]],
            "intensity_coverage":result["intensity_score"],
            "fom":fom_score(result,len(obs_d)),
            "element_coverage":round(len(set(fp["elements"])&set(elements))/max(len(set(elements)),1),2),
            "total_score":round(total_score,1),
            "preferred_orientation":result["preferred_orientation"],
        })
        for p in result["matched_pairs"]: covered_d.add(round(p[0],4))
    unmatched=[d for d in obs_d if d not in covered_d]
    return {"phases":phases_out,"unmatched_peaks":[round(d,4) for d in unmatched],
        "total_phases_found":len(phases_out),
        "coverage":round(len(covered_d)/len(obs_d),3) if obs_d else 0,
        "total_obs_peaks":len(obs_d)}
