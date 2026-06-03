#!/usr/bin/env python3
"""
pyprofex entry point — CLI interface for phase identification.
Build with: pyinstaller --onefile pyprofex/profex_cli.py
"""
import sys, os, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

def main():
    import argparse
    parser = argparse.ArgumentParser(description="pyprofex — XRD Phase Identification CLI")
    sub = parser.add_subparsers(dest="command")
    
    # search-match
    sm = sub.add_parser("search", help="Identify phases from d-spacings")
    sm.add_argument("d_spacings", type=float, nargs="+", help="Observed d-spacings in Å")
    sm.add_argument("--elements", "-e", type=str, default="", help="Element hints, comma-separated")
    sm.add_argument("--n", type=int, default=0, help="Expected number of phases")
    sm.add_argument("--method", choices=["iterative", "independent"], default="iterative")
    
    # list-db
    ld = sub.add_parser("list-db", help="List fingerprints in the database")
    ld.add_argument("--search", "-s", type=str, default="", help="Search by name")
    ld.add_argument("--limit", "-l", type=int, default=20)
    
    # db-info
    sub.add_parser("db-info", help="Show database statistics")
    
    # mcp-server
    mcp = sub.add_parser("mcp", help="Run MCP server")
    mcp.add_argument("--transport", choices=["stdio", "sse"], default="stdio")
    mcp.add_argument("--port", type=int, default=8100)
    
    # suggest-elements
    se = sub.add_parser("suggest-elements", help="Suggest elements from d-spacings")
    se.add_argument("d_spacings", type=float, nargs="+")
    
    args = parser.parse_args()
    
    if args.command == "search":
        from search_match import get_db, iterative_search_match, independent_score_all
        get_db()
        elements = [e.strip() for e in args.elements.split(",") if e.strip()]
        if args.method == "iterative":
            result = iterative_search_match(args.d_spacings, elements, n_expected=args.n)
        else:
            result = independent_score_all(args.d_spacings, elements, n_expected=args.n or 5)
        print(json.dumps(result, indent=2, default=str))
    
    elif args.command == "list-db":
        from search_match import get_db
        db = get_db()
        results = []
        for k, v in db.items():
            if args.search and args.search.lower() not in v["name"].lower():
                continue
            pk = [round(p[0], 4) for p in v["d_spacings"][:5]]
            results.append({"name": v["name"], "formula": v["formula"],
                            "n_peaks": len(v["d_spacings"]), "top_d": pk,
                            "source": v["_source"]})
            if len(results) >= args.limit:
                break
        print(json.dumps(results, indent=2))
    
    elif args.command == "db-info":
        from search_match import get_db
        db = get_db()
        sources = {}
        for v in db.values():
            s = v.get("_source", "unknown")
            sources[s] = sources.get(s, 0) + 1
        print(json.dumps({
            "total_fingerprints": len(db),
            "by_source": sources,
            "peak_stats": {
                "min": min(len(v["d_spacings"]) for v in db.values()),
                "max": max(len(v["d_spacings"]) for v in db.values()),
                "avg": round(sum(len(v["d_spacings"]) for v in db.values()) / len(db), 1),
            }
        }, indent=2))
    
    elif args.command == "suggest-elements":
        from search_match import suggest_elements
        els = suggest_elements(args.d_spacings)
        print(json.dumps({"suggested_elements": els}, indent=2))
    
    elif args.command == "mcp":
        from mcp_server import main as mcp_main
        mcp_main()
    
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
