#!/usr/bin/env python3
"""
build_unified_db.py — 把三个指纹源合并为一个预编译的 pickle 文件，
然后将加载入口改为从 pickle 快速加载（<1s）。

流程:
1. 从 fingerprints.py / fingerprints_auto.py / fingerprints_powcod.json 加载
2. 统一格式 (normalize)
3. 保存为 fingerprints_unified.pkl (pickle, Protocol 4)
4. search_match.py 改为优先加载 pickle
"""
import json, os, pickle, sys, time
from pathlib import Path

HERE = Path(__file__).parent
OUTPUT = HERE / "fingerprints_unified.pkl"

# ── 1. 手动库 ──
def load_manual():
    sys.path.insert(0, str(HERE))
    from fingerprints import FINGERPRINT_DB
    return {"manual_" + k: v for k, v in FINGERPRINT_DB.items()}

# ── 2. 自建 CIF 库 ──
def load_auto():
    try:
        with open(HERE / "fingerprints_auto.py") as f:
            code = f.read()
        exec(code, globals())
        fp = globals().get("FINGERPRINT_DB_AUTO") or globals().get("FINGERPRINT_DB", {})
        return dict(fp)
    except:
        return {}

# ── 3. POWCOD 库 ──
def load_powcod():
    path = HERE / "fingerprints_powcod.json"
    if not path.exists():
        return {}
    with open(path) as f:
        return json.load(f)

# ── 归一化 ──
_METAL = {"Li","Be","Na","Mg","Al","K","Ca","Sc","Ti","V","Cr","Mn","Fe","Co",
          "Ni","Cu","Zn","Rb","Sr","Y","Zr","Nb","Mo","Tc","Ru","Rh","Pd","Ag",
          "Cd","In","Sn","Sb","Cs","Ba","La","Ce","Pr","Nd","Pm","Sm","Eu",
          "Gd","Tb","Dy","Ho","Er","Tm","Yb","Lu","Hf","Ta","W","Re","Os","Ir",
          "Pt","Au","Hg","Tl","Pb","Bi","Po","At","Fr","Ra","Ac","Th","Pa","U",
          "Np","Pu","Am","Cm","Bk","Cf","Es","Fm","Md","No","Lr"}
_ORGANIC = {"C","H","N","S"}

def norm(entry, key="", source=""):
    name = entry.get("name") or entry.get("mineral") or entry.get("n") or key
    formula = entry.get("formula") or entry.get("chemical_formula") or entry.get("f") or ""
    
    peaks = entry.get("peaks") or entry.get("d_spacings") or entry.get("p", [])
    if isinstance(peaks, list) and peaks:
        first = peaks[0]
        if isinstance(first, (list, tuple)):
            d_spacings = [(round(p[0],4), round(p[1],1)) for p in peaks
                          if isinstance(p,(list,tuple)) and len(p)>=2]
        elif isinstance(first, dict):
            d_spacings = [(round(p.get("d",0),4), round(p.get("intensity",100),1)) for p in peaks]
        else:
            d_spacings = []
    else:
        d_spacings = []

    if not d_spacings and isinstance(peaks, dict):
        dl = peaks.get("d_spacings",[])
        il = peaks.get("intensities",[])
        if dl and il:
            d_spacings = [(round(d,4), round(i,1)) for d,i in zip(dl,il) if 0.5<=d<=100]

    if not d_spacings:
        return None
    
    d_spacings.sort(key=lambda x:x[1], reverse=True)
    
    import re as _re
    elements = entry.get("elements", [])
    if not elements and formula:
        elements = list(set(_re.findall(r'[A-Z][a-z]?', formula)))
        elements = [e for e in elements if e[0].isalpha() and e not in ('X','R')]
    
    return {
        "name": name, "formula": formula, "elements": elements,
        "d_spacings": d_spacings, "_source": source,
    }

def build():
    merged = {}
    
    # Manual
    for k, e in load_manual().items():
        n = norm(e, k, "manual")
        if n: merged[k] = n
    
    # Auto CIF
    for k, e in load_auto().items():
        n = norm(e, k, "auto_cif")
        if n:
            name_lower = n["name"].lower()
            if not any(v["name"].lower()==name_lower for v in merged.values()):
                merged[k] = n
    
    # POWCOD (filter organics)
    els = set()
    for k, e in load_powcod().items():
        n = norm(e, k, "powcod")
        if n:
            n["_source"] = "powcod"
            el_set = set(n.get("elements",[]))
            if (el_set & _ORGANIC) and not (el_set & _METAL):
                continue
            name_lower = n["name"].lower()
            if not any(v["name"].lower()==name_lower for v in merged.values()):
                safe_key = n["name"].lower().replace(" ","_").replace("-","_")
                safe_key = "".join(c for c in safe_key if c.isalnum() or c=="_")
                if not safe_key: safe_key = k
                if safe_key in merged: safe_key = f"{safe_key}_{len(merged)}"
                merged[safe_key] = n
    
    return merged

def main():
    print("Building unified fingerprint DB...")
    t0 = time.time()
    db = build()
    t1 = time.time()
    print(f"  {len(db)} entries ({t1-t0:.1f}s to build)")
    
    # Source stats
    sources = {}
    for v in db.values():
        s = v["_source"]
        sources[s] = sources.get(s,0)+1
    for s,c in sorted(sources.items()):
        print(f"  {s}: {c}")
    
    # Save as pickle
    pkl_bytes = pickle.dumps(db, protocol=pickle.HIGHEST_PROTOCOL)
    OUTPUT.write_bytes(pkl_bytes)
    print(f"\nWritten: {OUTPUT} ({len(pkl_bytes)//1024//1024} MB, {len(db)} entries)")
    
    # Test load speed
    t2 = time.time()
    db2 = pickle.loads(OUTPUT.read_bytes())
    t3 = time.time()
    print(f"Load test: {len(db2)} entries in {t3-t2:.3f}s")

if __name__ == "__main__":
    main()
