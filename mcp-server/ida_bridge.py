#!/usr/bin/env python3
"""
IDA MCP Bridge - Connects Claude Desktop (stdio) to IDA HTTP server

This script acts as an MCP server for Claude Desktop and forwards requests to IDA.

NOTE: This is ONLY for Claude Desktop MCP integration.
For multi-LLM support (Claude/OpenAI/Gemini), use the standalone ImGui client instead.
"""

import sys
import json
import asyncio
import urllib.request
import urllib.error
from typing import Any

# MCP protocol constants
IDA_HTTP_HOST = "127.0.0.1"
IDA_HTTP_PORT = 13120


def http_get(path: str) -> dict:
    """Make HTTP GET request to IDA server"""
    url = f"http://{IDA_HTTP_HOST}:{IDA_HTTP_PORT}{path}"
    try:
        with urllib.request.urlopen(url, timeout=30) as resp:
            return json.loads(resp.read().decode())
    except urllib.error.URLError as e:
        return {"error": f"Cannot connect to IDA: {e}"}
    except Exception as e:
        return {"error": str(e)}


def http_post(path: str, data: dict) -> dict:
    """Make HTTP POST request to IDA server"""
    url = f"http://{IDA_HTTP_HOST}:{IDA_HTTP_PORT}{path}"
    try:
        req = urllib.request.Request(
            url,
            data=json.dumps(data).encode(),
            headers={"Content-Type": "application/json"},
            method="POST"
        )
        with urllib.request.urlopen(req, timeout=60) as resp:
            return json.loads(resp.read().decode())
    except urllib.error.URLError as e:
        return {"error": f"Cannot connect to IDA: {e}"}
    except Exception as e:
        return {"error": str(e)}


def check_ida_connection() -> bool:
    """Check if IDA HTTP server is running"""
    result = http_get("/health")
    return result.get("status") == "ok"


def get_tools_from_ida() -> list:
    """Get tool definitions from IDA"""
    result = http_get("/tools")
    return result.get("tools", [])


def call_ida_tool(name: str, arguments: dict) -> dict:
    """Call a tool on IDA server"""
    result = http_post("/call", {"name": name, "arguments": arguments})
    return result.get("result", result)


class MCPBridge:
    """MCP protocol bridge between stdio and IDA HTTP"""

    def __init__(self):
        self.tools = []
        self.request_id = 0

    def send_response(self, id: Any, result: Any):
        """Send JSON-RPC response"""
        response = {
            "jsonrpc": "2.0",
            "id": id,
            "result": result
        }
        self.write_message(response)

    def send_error(self, id: Any, code: int, message: str):
        """Send JSON-RPC error"""
        response = {
            "jsonrpc": "2.0",
            "id": id,
            "error": {"code": code, "message": message}
        }
        self.write_message(response)

    def write_message(self, msg: dict):
        """Write message to stdout"""
        data = json.dumps(msg)
        sys.stdout.write(data + "\n")
        sys.stdout.flush()

    def handle_initialize(self, id: Any, params: dict):
        """Handle initialize request"""
        self.send_response(id, {
            "protocolVersion": "2024-11-05",
            "capabilities": {
                "tools": {}
            },
            "serverInfo": {
                "name": "ida-re-assistant",
                "version": "1.0.0"
            }
        })

    def handle_initialized(self, id: Any, params: dict):
        """Handle initialized notification"""
        pass  # No response needed for notifications

    def handle_tools_list(self, id: Any, params: dict):
        """Handle tools/list request"""
        # Try to get tools from IDA
        ida_tools = get_tools_from_ida()

        if not ida_tools:
            # Fallback tools if IDA not connected
            ida_tools = [
                {
                    "name": "check_ida_connection",
                    "description": "Check if IDA Pro is connected",
                    "inputSchema": {"type": "object", "properties": {}}
                }
            ]

        # Convert to MCP format
        tools = []
        for t in ida_tools:
            tools.append({
                "name": t["name"],
                "description": t.get("description", ""),
                "inputSchema": t.get("inputSchema", {"type": "object", "properties": {}})
            })

        self.send_response(id, {"tools": tools})

    def handle_tools_call(self, id: Any, params: dict):
        """Handle tools/call request"""
        name = params.get("name", "")
        arguments = params.get("arguments", {})

        # Special case: check connection
        if name == "check_ida_connection":
            connected = check_ida_connection()
            result_text = json.dumps({
                "connected": connected,
                "message": "IDA Pro is connected" if connected else "IDA Pro is not connected. Start IDA and run the MCP plugin (Ctrl+Shift+M)"
            }, indent=2)
            self.send_response(id, {
                "content": [{"type": "text", "text": result_text}]
            })
            return

        # Call IDA tool
        result = call_ida_tool(name, arguments)
        result_text = json.dumps(result, indent=2)

        self.send_response(id, {
            "content": [{"type": "text", "text": result_text}]
        })

    def handle_request(self, msg: dict):
        """Handle incoming JSON-RPC request"""
        id = msg.get("id")
        method = msg.get("method", "")
        params = msg.get("params", {})

        if method == "initialize":
            self.handle_initialize(id, params)
        elif method == "notifications/initialized" or method == "initialized":
            self.handle_initialized(id, params)
        elif method == "tools/list":
            self.handle_tools_list(id, params)
        elif method == "tools/call":
            self.handle_tools_call(id, params)
        elif method == "ping":
            self.send_response(id, {})
        else:
            # Unknown method
            if id is not None:
                self.send_error(id, -32601, f"Method not found: {method}")

    def run(self):
        """Main loop - read from stdin, process, write to stdout"""
        # Check IDA connection at startup
        if check_ida_connection():
            sys.stderr.write("[IDA-Bridge] Connected to IDA Pro\n")
            sys.stderr.flush()
        else:
            sys.stderr.write("[IDA-Bridge] Warning: IDA Pro not connected. Start IDA and run the plugin.\n")
            sys.stderr.flush()

        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue

            try:
                msg = json.loads(line)
                self.handle_request(msg)
            except json.JSONDecodeError as e:
                sys.stderr.write(f"[IDA-Bridge] JSON parse error: {e}\n")
                sys.stderr.flush()
            except Exception as e:
                sys.stderr.write(f"[IDA-Bridge] Error: {e}\n")
                sys.stderr.flush()


def main():
    bridge = MCPBridge()
    bridge.run()


if __name__ == "__main__":
    main()
