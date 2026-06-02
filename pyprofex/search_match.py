#!/usr/bin/env python3
"""
pyprofex.search_match — Search-Match 引擎
提供 independent_score_all（推荐）和 iterative_search_match（备用）
"""
from __future__ import annotations
import math
from fingerprints import FINGERPRINT_DB, get_by_elements

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
    matched = set()
    for ref_d,_ in sorted(ref_di, key=lambda x:x[1], reverse=True):
        for oi,od in enumerate(obs_d):
            if oi not in matched and match_d(od,ref_d,tol): matched.add(oi); break
    return [d for i,d in enumerate(obs_d) if i not in matched]

def iterative_search_match(obs_d, elements, n_expected=0, tol=MATCH_TOLERANCE, min_peaks=2):
    candidates = get_by_elements(elements)
    if not candidates:
        return {"phases":[],"unmatched_peaks":obs_d,"total_phases_found":0,"coverage":0}
    remaining = list(obs_d); identified = []; total_obs = len(obs_d)
    for _ in range(n_expected if n_expected>0 else 10):
        if len(remaining)<min_peaks: break
        scored = []
        for name,fp in candidates:
            if any(p["name"]==name for p in identified): continue
            result = score_single_phase(remaining, fp["d_spacings"], tol)
            if result["matches"]<min_peaks: continue
            fom_v = fom_score(result, len(remaining))
            elem_cov = len(set(fp["elements"])&set(elements))/max(len(set(elements)),1)
            scored.append((fom_v*10+elem_cov*3, name, fp, result))
        if not scored: break
        scored.sort(key=lambda x:x[0], reverse=True)
        _, name, fp, result = scored[0]
        identified.append({"name":name,"formula":fp["formula"],"matches":result["matches"],
            "matched_d":[round(p[0],4) for p in result["matched_pairs"]],
            "intensity_coverage":result["intensity_score"],
            "fom":fom_score(result,len(remaining)),
            "element_coverage":round(len(set(fp["elements"])&set(elements))/max(len(set(elements)),1),2),
            "preferred_orientation":result["preferred_orientation"]})
        remaining = subtract_phase(remaining, fp["d_spacings"], tol)
        if n_expected>0 and len(identified)>=n_expected: break
    return {"phases":identified,"unmatched_peaks":[round(d,4) for d in remaining],
        "total_phases_found":len(identified),
        "coverage":round((total_obs-len(remaining))/total_obs,3) if total_obs else 0,
        "total_obs_peaks":total_obs}

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
