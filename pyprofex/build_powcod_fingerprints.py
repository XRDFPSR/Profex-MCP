#!/usr/bin/env python3
"""Build compact POWCOD database as JSON + lazy loader."""
import sqlite3, json, sys, os, struct
from pathlib import Path

SQ_PATH = "/workspace/profex/COD/cod2205ino.sq"
OUTPUT = Path(__file__).parent / "fingerprints_powcod.json"

def build():
    conn = sqlite3.connect(SQ_PATH)
    conn.text_factory = bytes
    cur = conn.cursor()
    
    cur.execute("""
        SELECT id, name, mineralname, chemical_formula, spacegroup, rir, 
               nreflections, dvalue, intensita 
        FROM id 
        WHERE mineralname IS NOT NULL AND mineralname != '' AND mineralname != '?'
    """)
    
    entries = {}
    count = 0
    
    for r in cur:
        cid, name, mineral, formula, sg, rir, nref, dblob, iblob = r
        
        mineral_str = mineral.decode('utf-8', errors='replace').strip()
        if not mineral_str:
            continue
        
        # Parse d and I as comma-separated text
        d_str = dblob.decode('utf-8', errors='replace') if dblob else ""
        d_vals = [float(x) for x in d_str.split(',') if x.strip()]
        
        i_str = iblob.decode('utf-8', errors='replace') if iblob else ""
        i_vals = [float(x) for x in i_str.split(',') if x.strip()]
        
        # Build valid peak pairs
        peaks = []
        imax = 0
        for j in range(min(len(d_vals), len(i_vals))):
            if 0.5 <= d_vals[j] <= 100:
                if i_vals[j] > imax:
                    imax = i_vals[j]
                peaks.append([round(d_vals[j], 4), i_vals[j]])
        
        if len(peaks) < 3 or len(peaks) > 150:  # 排除峰数过少的 和 过多的（超多峰干扰）
            continue
        
        # Normalize intensities
        if imax > 0:
            peaks = [[d, round(i / imax * 100, 1)] for d, i in peaks]
        
        name_str = name.decode('utf-8', errors='replace').strip() if name else ""
        formula_str = formula.decode('utf-8', errors='replace').strip('\'" ') if formula else ""
        sg_str = sg.decode('utf-8', errors='replace').strip() if sg else ""
        cod_id = cid.decode('utf-8') if isinstance(cid, bytes) else str(cid)
        rir_val = float(rir) if rir else 0.0
        
        key = mineral_str.lower().replace(" ", "_").replace("-", "_")
        key = "".join(c for c in key if c.isalnum() or c == "_")
        
        if key in entries:
            key = f"{key}_{cod_id}"
        
        entries[key] = {
            "n": mineral_str,
            "f": formula_str,
            "s": sg_str,
            "c": cod_id,
            "r": rir_val,
            "np": len(peaks),
            "p": peaks,
        }
        
        count += 1
        if count % 3000 == 0:
            print(f"  {count}...")
    
    conn.close()
    print(f"Total: {count}")
    return entries


def main():
    print("Building compact POWCOD JSON...")
    entries = build()
    
    json_bytes = json.dumps(entries, separators=(',', ':')).encode('utf-8')
    OUTPUT.write_bytes(json_bytes)
    
    print(f"Written: {OUTPUT} ({len(json_bytes):,} bytes, {len(entries)} entries)")


if __name__ == "__main__":
    main()
