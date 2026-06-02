#!/usr/bin/env python3
"""Test the Profex MCP Server using the MCP client library."""

import asyncio
import json
import subprocess
import sys
from pathlib import Path


async def test_mcp_server():
    """Start the MCP server and test tool/resource listing."""
    script_dir = Path(__file__).resolve().parent.parent
    server_script = script_dir / "mcp_server.py"
    assert server_script.exists(), f"Not found: {server_script}"

    # Start server subprocess
    proc = await asyncio.create_subprocess_exec(
        sys.executable, str(server_script),
        stdin=asyncio.subprocess.PIPE,
        stdout=asyncio.subprocess.PIPE,
        stderr=asyncio.subprocess.PIPE,
    )

    async def send_req(method: str, params: dict | None = None, req_id: int = 1):
        req = json.dumps({
            "jsonrpc": "2.0",
            "id": req_id,
            "method": method,
            "params": params or {},
        })
        proc.stdin.write((req + "\n").encode())
        await proc.stdin.drain()

        # Read response (proper MCP responses are multi-line JSON-RPC)
        response_lines = []
        line = await asyncio.wait_for(proc.stdout.readline(), timeout=10)
        response_lines.append(line.decode().strip())
        return response_lines[0]

    try:
        # Step 1: Initialize
        print("1️⃣  Testing initialize...")
        resp = await send_req("initialize", {
            "protocolVersion": "2024-11-05",
            "capabilities": {},
            "clientInfo": {"name": "test-client", "version": "1.0"},
        }, 1)
        print(f"   → {resp[:200]}...")

        # Step 2: tools/list
        print("\n2️⃣  Testing tools/list...")
        resp = await send_req("tools/list", {}, 2)
        if resp:
            data = json.loads(resp)
            tools = data.get("result", {}).get("tools", [])
            print(f"   → Found {len(tools)} tools:")
            for t in tools:
                print(f"      ✅ {t['name']}")

        # Step 3: resources/list
        print("\n3️⃣  Testing resources/list...")
        resp = await send_req("resources/list", {}, 3)
        if resp:
            data = json.loads(resp)
            resources = data.get("result", {}).get("resources", [])
            print(f"   → Found {len(resources)} resources")

        # Step 4: available_formats
        print("\n4️⃣  Testing call_tool(available_formats)...")
        resp = await send_req(
            "tools/call",
            {"name": "available_formats", "arguments": {}},
            4,
        )
        if resp:
            data = json.loads(resp)
            content = data.get("result", {}).get("content", [])
            if content:
                result_json = json.loads(content[0]["text"])
                print(f"   → Import formats: {len(result_json['import_formats'])}")
                print(f"   → Export formats: {len(result_json['export_formats'])}")

        print("\n✅ All MCP Server tests passed!")

    finally:
        proc.stdin.close()
        try:
            await asyncio.wait_for(proc.wait(), timeout=5)
        except asyncio.TimeoutError:
            proc.kill()


if __name__ == "__main__":
    asyncio.run(test_mcp_server())
