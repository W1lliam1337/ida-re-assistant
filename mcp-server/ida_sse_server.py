#!/usr/bin/env python3
"""
IDA MCP SSE Server - HTTP/SSE transport for multi-LLM support

This server provides HTTP/SSE transport for MCP protocol, allowing any
MCP-compatible client (ChatGPT, Claude, Gemini, etc.) to connect.

For Claude Desktop stdio integration, use ida_bridge.py instead.
"""

import json
import urllib.request
import urllib.error
from typing import Any
import asyncio
from contextlib import asynccontextmanager

try:
    from mcp.server import Server
    from mcp.types import Tool, TextContent
    from mcp.server.sse import SseServerTransport
    from starlette.applications import Starlette
    from starlette.routing import Route
    from starlette.responses import Response
    from starlette.requests import Request
    import uvicorn
except ImportError as e:
    print(f"Missing dependencies. Install with: pip install mcp starlette uvicorn sse-starlette")
    print(f"Error: {e}")
    exit(1)

# IDA HTTP server config
IDA_HTTP_HOST = "127.0.0.1"
IDA_HTTP_PORT = 13120
SSE_SERVER_PORT = 13121


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


# Create MCP server
mcp_server = Server("ida-re-assistant")


@mcp_server.list_tools()
async def list_tools() -> list[Tool]:
    """List available tools from IDA"""
    ida_tools = get_tools_from_ida()

    if not ida_tools:
        # Fallback if IDA not connected
        return [
            Tool(
                name="check_ida_connection",
                description="Check if IDA Pro is connected",
                inputSchema={"type": "object", "properties": {}}
            )
        ]

    # Convert IDA tools to MCP format
    tools = []
    for t in ida_tools:
        tools.append(
            Tool(
                name=t["name"],
                description=t.get("description", ""),
                inputSchema=t.get("inputSchema", {"type": "object", "properties": {}})
            )
        )

    return tools


@mcp_server.call_tool()
async def call_tool(name: str, arguments: dict) -> list[TextContent]:
    """Call a tool on IDA server"""

    # Special case: check connection
    if name == "check_ida_connection":
        connected = check_ida_connection()
        result = {
            "connected": connected,
            "message": "IDA Pro is connected" if connected else
                      "IDA Pro is not connected. Start IDA and run the MCP plugin (Ctrl+Shift+M)"
        }
        return [TextContent(type="text", text=json.dumps(result, indent=2))]

    # Call IDA tool
    result = call_ida_tool(name, arguments)
    return [TextContent(type="text", text=json.dumps(result, indent=2))]


# Starlette app for SSE transport
async def handle_sse(request: Request):
    """Handle SSE connection"""
    async with SseServerTransport("/messages") as (read_stream, write_stream):
        await mcp_server.run(
            read_stream,
            write_stream,
            mcp_server.create_initialization_options()
        )
    return Response()


async def handle_messages(request: Request):
    """Handle incoming messages"""
    return Response()


@asynccontextmanager
async def lifespan(app: Starlette):
    """Lifespan handler"""
    print(f"[IDA-SSE-Server] Starting on port {SSE_SERVER_PORT}")
    if check_ida_connection():
        print("[IDA-SSE-Server] Connected to IDA Pro")
    else:
        print("[IDA-SSE-Server] Warning: IDA Pro not connected")
    yield
    print("[IDA-SSE-Server] Shutting down")


app = Starlette(
    debug=True,
    routes=[
        Route("/sse", endpoint=handle_sse),
        Route("/messages", endpoint=handle_messages, methods=["POST"]),
    ],
    lifespan=lifespan
)


def main():
    """Start SSE server"""
    print(f"""
IDA RE Assistant MCP Server (SSE Transport)
===========================================
Server URL: http://127.0.0.1:{SSE_SERVER_PORT}/sse
IDA HTTP: http://{IDA_HTTP_HOST}:{IDA_HTTP_PORT}

This server supports ANY MCP-compatible client:
- ChatGPT with MCP plugins
- Claude with MCP support
- Gemini with MCP support
- Custom MCP clients

For Claude Desktop stdio integration, use ida_bridge.py instead.
""")

    uvicorn.run(
        app,
        host="127.0.0.1",
        port=SSE_SERVER_PORT,
        log_level="info"
    )


if __name__ == "__main__":
    main()
