#!/usr/bin/env python3
"""
pyprofex.build_fingerprints — 从 COD db3 数据库 + CIF 文件自动构建指纹库

工作流程:
1. 读取 cod-260101.db3，筛选有矿物名 / 质量好的条目
2. 按优先级排序（有矿物名优先，R 因子低优先）
3. 去重（同矿物名取最优条目）
4. 下载 CIF → 计算粉末衍射花样（含对称操作扩展 → 结构因子 → 强度）
5. 输出 fingerprints_auto.py

使用方式:
  python build_fingerprints.py                            # 默认构建 5000 个常见矿物
  python build_fingerprints.py --max 2000                  # 只构建 2000 个
  python build_fingerprints.py --minerals Quartz,Calcite   # 指定矿物
  python build_fingerprints.py --cod-id 1011097           # 指定 COD ID
  python build_fingerprints.py --db3 /path/to/cod.db3     # 指定 db3 路径
  python build_fingerprints.py --skip-download             # 用缓存 CIF（不下载）
  python build_fingerprints.py --full                      # 全部 530K 条目
"""

from __future__ import annotations

import json
import math
import os
import sqlite3
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any

# Import the CIF diffraction engine
from cif2fingerprint import CifParser, calculate_powder_pattern, get_cif, CIF_CACHE_DIR

# ══════════════════════════════════════════════════════════════════════
# DB3 数据库接口
# ══════════════════════════════════════════════════════════════════════

DEFAULT_DB3 = "/workspace/profex/cod-260101.db3"
CIF_CACHE_DIR = Path(__file__).parent / "cif_cache"
CIF_CACHE_DIR.mkdir(exist_ok=True)

# 已知矿物黑名单 — COD 中同名的非矿物条目
MINERAL_BLACKLIST = {
    "Synthetic", "synthetic", "simulated", "analogue",
}


def open_db(db_path: str | Path = DEFAULT_DB3) -> sqlite3.Connection:
    """Open the COD db3 database."""
    conn = sqlite3.connect(str(db_path))
    conn.row_factory = sqlite3.Row
    return conn


def get_candidates(
    conn: sqlite3.Connection,
    max_entries: int = 0,
    min_robs: float | None = 0.15,
    minerals_only: bool = True,
) -> list[sqlite3.Row]:
    """
    筛选候选 COD 条目。

    Args:
        conn: db3 连接
        max_entries: 最大条目数（0=全部）
        min_robs: R 因子上限（None=不过滤）
        minerals_only: 只选有矿物名的条目

    Returns:
        按优先级排序的行列表
    """
    conditions = []
    params = []

    if minerals_only:
        conditions.append("mineral IS NOT NULL AND mineral != '' AND mineral != '\\N'")
    
    if min_robs is not None:
        conditions.append("(Robs IS NULL OR Robs < ?)")
        params.append(min_robs)
    
    # 必须有晶胞参数
    conditions.append("a > 0 AND b > 0 AND c > 0")
    
    # 必须有空间群
    conditions.append("sgNumber > 0")
    
    # 排除黑名单
    conditions.append("(mineral IS NULL OR mineral NOT IN ("
                      + ",".join("?" for _ in MINERAL_BLACKLIST) + "))")
    params.extend(MINERAL_BLACKLIST)

    where = " AND ".join(conditions)
    
    # 按优先级排序: 有矿物名 > Robs 低 > 有 Z 值 > 有 commonname
    order = """
        CASE WHEN mineral IS NOT NULL AND mineral != '' AND mineral != '\\N' THEN 0 ELSE 1 END,
        CASE WHEN commonname IS NOT NULL AND commonname != '' AND commonname != '\\N' THEN 0 ELSE 1 END,
        CASE WHEN Z > 0 THEN 0 ELSE 1 END,
        Robs ASC NULLS LAST
    """
    
    sql = f"SELECT * FROM data WHERE {where} ORDER BY {order}"
    
    if max_entries > 0:
        sql += f" LIMIT {max_entries}"
    
    cur = conn.execute(sql, params)
    return cur.fetchall()


def dedup_by_mineral(candidates: list[sqlite3.Row]) -> list[sqlite3.Row]:
    """
    按矿物名去重：同名矿物只保留第一个（优先级已排序）。
    返回去重后的列表 + 统计信息。
    """
    seen: dict[str, sqlite3.Row] = {}
    seen_order: list[str] = []
    mineral_count = 0
    non_mineral_count = 0
    
    for row in candidates:
        mineral = row["mineral"]
        if mineral and mineral != "\\N" and mineral.strip():
            mineral_count += 1
            key = mineral.strip().lower()
            if key not in seen:
                seen[key] = row
                seen_order.append(key)
        else:
            non_mineral_count += 1
    
    result = [seen[k] for k in seen_order]
    # 如果 candidates 有 mineral=NULL 的条目且 max_entries 足够，也保留
    # （但在 minerals_only=True 下不会出现）
    return result, {"mineral_entries": mineral_count, "non_mineral": non_mineral_count, "unique_minerals": len(result)}


# ══════════════════════════════════════════════════════════════════════
# 指纹生成
# ══════════════════════════════════════════════════════════════════════

def generate_fingerprint_from_db_row(row: sqlite3.Row, d_min: float = 0.8, top_n: int = 20) -> dict | None:
    """
    从 db3 行生成指纹。
    1. 用 get_cif() 下载 CIF 文件（或从缓存读取）
    2. 计算粉末衍射花样
    3. 返回结构化指纹
    """
    entry_id = str(row["file"])
    
    # 获取 CIF
    cif = get_cif(entry_id)
    if not cif:
        return {"error": f"CIF not found for {entry_id}", "entry_id": entry_id, "file": row["file"]}
    
    # 计算粉末衍射
    parser = CifParser(cif)
    if parser.a == 0:
        return {"error": f"Invalid cell for {entry_id}", "entry_id": entry_id, "file": row["file"]}
    
    pattern = calculate_powder_pattern(cif, d_min=d_min, top_n=top_n)
    if not pattern:
        return {"error": f"No peaks for {entry_id}", "entry_id": entry_id, "file": row["file"]}
    
    # 提取元素
    elements = list(set(a.get("element", "?") for a in parser.atoms if a.get("element")))
    elements = [e for e in elements if e and e[0].isalpha()]
    
    mineral = row["mineral"] if row["mineral"] and row["mineral"] != "\\N" else ""
    formula = row["formula"] if row["formula"] and row["formula"] != "\\N" else ""
    
    # 构建指纹
    fingerprint: dict[str, Any] = {
        "entry_id": entry_id,
        "mineral": mineral,
        "formula": formula.strip("- "),
        "sg": parser.sg_hm or f"#{parser.sg_number}",
        "sg_number": parser.sg_number,
        "a": round(parser.a, 4),
        "b": round(parser.b, 4),
        "c": round(parser.c, 4),
        "alpha": round(parser.alpha, 2),
        "beta": round(parser.beta, 2),
        "gamma": round(parser.gamma, 2),
        "Z": parser.Z,
        "elements": sorted(elements),
        "n_atoms": len(parser.atoms),
        "peaks": [
            {
                "d": round(p["d"], 4),
                "intensity": round(p["intensity_rel"], 1),
                "h": p["h"], "k": p["k"], "l": p["l"],
                "two_theta": round(p["two_theta"], 3),
            }
            for p in pattern
        ],
    }
    return fingerprint


# ══════════════════════════════════════════════════════════════════════
# 批量生成（多线程加速）
# ══════════════════════════════════════════════════════════════════════

def generate_fingerprint_for_row(args):
    """用于多线程的辅助函数。"""
    row, d_min, top_n = args
    mineral_key = row["mineral"].strip() if row["mineral"] and row["mineral"] != "\\N" else f"COD_{row['file']}"
    result = generate_fingerprint_from_db_row(row, d_min=d_min, top_n=top_n)
    return mineral_key, result


def generate_batch(
    db_path: str = DEFAULT_DB3,
    max_entries: int = 5000,
    min_robs: float | None = None,
    minerals_only: bool = True,
    dedup: bool = True,
    d_min: float = 0.8,
    top_n: int = 20,
    skip_download: bool = False,
    verbose: bool = True,
    max_workers: int = 8,
) -> dict[str, dict]:
    """
    批量生成指纹库（多线程加速）。

    Returns:
        {矿物名: 指纹数据} 字典
    """
    if verbose:
        print(f"Opening db3: {db_path}")
    
    conn = open_db(db_path)
    
    if verbose:
        print(f"Selecting candidates (max={max_entries}, min_robs={min_robs})...")
    
    candidates = get_candidates(conn, max_entries=max_entries, min_robs=min_robs, minerals_only=minerals_only)
    
    if verbose:
        print(f"  Total candidates: {len(candidates)}")
    
    if dedup:
        candidates, stats = dedup_by_mineral(candidates)
        if verbose:
            print(f"  Unique minerals: {stats['unique_minerals']}")
    
    conn.close()

    if not candidates:
        if verbose:
            print("No candidates found!")
        return {}
    
    # 准备参数
    fingerprints: dict[str, dict] = {}
    errors: list[dict] = []
    total = len(candidates)
    
    for i, row in enumerate(candidates):
        mineral_key = row["mineral"].strip() if row["mineral"] and row["mineral"] != "\\N" else f"COD_{row['file']}"
        key = mineral_key
        if key in fingerprints:
            key = f"{key}_{row['file']}"
        
        result = generate_fingerprint_from_db_row(row, d_min=d_min, top_n=top_n)
        
        if "error" in result:
            errors.append(result)
        else:
            fingerprints[key] = result
        
        if (i + 1) % 20 == 0 or (i + 1) == total:
            msg = f"  [{i+1}/{total}] {i+1}/{total*100//total}% — {mineral_key} ({len(fingerprints)} OK, {len(errors)} err)"
            print(msg, flush=True)
    
    if verbose:
        print(f"\nDone: {len(fingerprints)} fingerprints, {len(errors)} errors")
    
    # 输出错误摘要
    if errors and verbose:
        error_types: dict[str, int] = {}
        for e in errors:
            et = e["error"].split(":")[0] if ":" in e["error"] else e["error"][:30]
            error_types[et] = error_types.get(et, 0) + 1
        print(f"Error summary:")
        for et, cnt in sorted(error_types.items(), key=lambda x: -x[1])[:10]:
            print(f"  {et}: {cnt}")
    
    return fingerprints


# ══════════════════════════════════════════════════════════════════════
# 输出
# ══════════════════════════════════════════════════════════════════════

def export_to_python(fingerprints: dict[str, dict], output_path: str | Path) -> None:
    """
    将指纹数据导出为 Python 模块格式（兼容 fingerprints.py 的 FINGERPRINT_DB 格式）。
    """
    lines = [
        '#!/usr/bin/env python3',
        '"""',
        f'pyprofex.fingerprints_auto — 自动生成指纹库 ({len(fingerprints)} 矿物)',
        '数据来源: COD CIF 理论计算，含对称操作扩展 + 结构因子强度',
        '"""',
        'from __future__ import annotations',
        '',
        f'# 自动生成，共 {len(fingerprints)} 个指纹条目',
        '',
        'FINGERPRINT_DB_AUTO: dict[str, dict] = {',
    ]
    
    for mineral_name in sorted(fingerprints.keys()):
        fp = fingerprints[mineral_name]
        entry_id = fp.get("entry_id", "")
        mineral = fp.get("mineral", mineral_name)
        formula = fp.get("formula", "")
        elements = fp.get("elements", [])
        peaks = fp.get("peaks", [])
        
        lines.append(f'    "{mineral_name}": {{')
        lines.append(f'        "formula": {json.dumps(formula)},')
        lines.append(f'        "cod_id": {json.dumps(entry_id)},')
        lines.append(f'        "mineral": {json.dumps(mineral)},')
        lines.append(f'        "elements": {json.dumps(elements)},')
        lines.append(f'        "sg": {json.dumps(fp.get("sg", ""))},')
        lines.append(f'        "sg_number": {fp.get("sg_number", 0)},')
        lines.append(f'        "a": {fp.get("a", 0)}, "b": {fp.get("b", 0)}, "c": {fp.get("c", 0)},')
        lines.append(f'        "Z": {fp.get("Z", 0)},')
        
        # d_spacings as list of (d, intensity) for backward compatibility
        d_spacings = [(p["d"], p["intensity"]) for p in peaks]
        ds_str = ", ".join(f"({d:.4f}, {i:.1f})" for d, i in d_spacings)
        lines.append(f'        "d_spacings": [{ds_str}],')
        
        # Full peaks for MCP output
        peaks_str = json.dumps(peaks, indent=12).replace("\n", "\n        ")
        lines.append(f'        "peaks": {peaks_str},')
        
        lines.append('    },')
    
    lines.append('}')
    lines.append('')
    lines.append(f'# Total: {len(fingerprints)} fingerprints')
    
    content = "\n".join(lines)
    
    output_path = Path(output_path)
    output_path.write_text(content)
    print(f"Written to {output_path} ({len(content)} bytes, {len(fingerprints)} entries)")


# ══════════════════════════════════════════════════════════════════════
# CLI
# ══════════════════════════════════════════════════════════════════════

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="从 COD db3 自动构建指纹库")
    parser.add_argument("--max", type=int, default=5000, help="最大条目数 (default: 5000)")
    parser.add_argument("--full", action="store_true", help="处理全部条目（覆盖 --max）")
    parser.add_argument("--minerals", type=str, default="", help="逗号分隔的矿物名列表")
    parser.add_argument("--cod-id", type=str, default="", help="逗号分隔的 COD ID 列表")
    parser.add_argument("--db3", type=str, default=DEFAULT_DB3, help=f"db3 路径 (default: {DEFAULT_DB3})")
    parser.add_argument("--output", type=str, default="fingerprints_auto.py", help="输出文件")
    parser.add_argument("--skip-download", action="store_true", help="跳过下载（只用缓存）")
    parser.add_argument("--d-min", type=float, default=0.8, help="最小 d-spacing (default: 0.8)")
    parser.add_argument("--top-n", type=int, default=20, help="每矿物保留峰数 (default: 20)")
    parser.add_argument("--verbose", action="store_true", help="详细输出")
    parser.add_argument("--min-robs", type=float, default=None, help="R 因子上限 (default: no filter)")
    parser.add_argument("--no-dedup", action="store_true", help="不去重（保留同矿物多个条目）")
    parser.add_argument("--workers", type=int, default=8, help="多线程数 (default: 8)")
    
    args = parser.parse_args()
    
    max_entries = 0 if args.full else args.max
    
    # 如果指定了矿物列表或 COD ID，则直接处理这些
    if args.minerals or args.cod_id:
        conn = open_db(args.db3)
        cur = conn.cursor()
        
        if args.minerals:
            mineral_list = [m.strip() for m in args.minerals.split(",")]
            placeholders = ",".join("?" for _ in mineral_list)
            rows = cur.execute(
                f"SELECT * FROM data WHERE mineral IN ({placeholders}) ORDER BY Robs ASC",
                mineral_list
            ).fetchall()
        elif args.cod_id:
            id_list = [i.strip() for i in args.cod_id.split(",")]
            placeholders = ",".join("?" for _ in id_list)
            rows = cur.execute(
                f"SELECT * FROM data WHERE file IN ({placeholders})",
                id_list
            ).fetchall()
        
        conn.close()
        
        print(f"Processing {len(rows)} specific entries...")
        candidates, _ = dedup_by_mineral(rows)
    else:
        candidates = []
    
    fingerprints = {}
    
    if candidates:
        # 处理指定条目
        for i, row in enumerate(candidates):
            mineral_key = row["mineral"].strip() if row["mineral"] and row["mineral"] != "\\N" else f"COD_{row['file']}"
            if args.verbose:
                print(f"  [{i+1}/{len(candidates)}] {mineral_key}")
            result = generate_fingerprint_from_db_row(row, d_min=args.d_min, top_n=args.top_n)
            if "error" not in result:
                fingerprints[mineral_key] = result
            elif args.verbose:
                print(f"    Error: {result['error']}")
    else:
        # 批量生成
        fingerprints = generate_batch(
            db_path=args.db3,
            max_entries=max_entries,
            min_robs=args.min_robs,
            minerals_only=True,
            dedup=not args.no_dedup,
            d_min=args.d_min,
            top_n=args.top_n,
            verbose=True,
            max_workers=args.workers,
        )
    
    # 导出
    if fingerprints:
        export_to_python(fingerprints, args.output)
        print(f"Success: {len(fingerprints)} fingerprints generated")
    else:
        print("No fingerprints generated!", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
