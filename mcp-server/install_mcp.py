#!/usr/bin/env python3
"""
Install IDA RE Assistant MCP server for various AI clients

Supports:
- Claude Desktop (stdio transport)
- ChatGPT, Gemini, and other MCP clients (SSE transport)
- Generic MCP client configuration

For standalone client without MCP, use the ImGui client (supports all LLM providers).
"""

import os
import sys
import json
from pathlib import Path
from typing import Optional

def get_claude_config_path():
    """Get Claude Desktop config path for current OS"""
    if sys.platform == "win32":
        appdata = os.environ.get("APPDATA", "")
        return Path(appdata) / "Claude" / "claude_desktop_config.json"
    elif sys.platform == "darwin":
        return Path.home() / "Library" / "Application Support" / "Claude" / "claude_desktop_config.json"
    else:  # Linux
        return Path.home() / ".config" / "Claude" / "claude_desktop_config.json"

def get_python_path():
    """Get current Python executable path"""
    return sys.executable

def get_server_path():
    """Get MCP server script path (the bridge that connects to IDA)"""
    script_dir = Path(__file__).parent.resolve()
    return script_dir / "mcp-server" / "ida_bridge.py"

def install():
    config_path = get_claude_config_path()
    python_path = get_python_path()
    server_path = get_server_path()

    if not server_path.exists():
        print(f"Error: Server not found at {server_path}")
        return False

    # Create config directory if needed
    config_path.parent.mkdir(parents=True, exist_ok=True)

    # Load existing config or create new
    config = {}
    if config_path.exists():
        try:
            with open(config_path, "r", encoding="utf-8") as f:
                config = json.load(f)
        except json.JSONDecodeError:
            print(f"Warning: Invalid JSON in {config_path}, creating new config")
            config = {}

    # Add/update MCP server
    if "mcpServers" not in config:
        config["mcpServers"] = {}

    config["mcpServers"]["ida-re-assistant"] = {
        "command": str(python_path),
        "args": [str(server_path)]
    }

    # Save config
    with open(config_path, "w", encoding="utf-8") as f:
        json.dump(config, f, indent=2)

    print(f"Successfully installed IDA RE Assistant MCP server!")
    print(f"Config: {config_path}")
    print(f"Python: {python_path}")
    print(f"Server: {server_path}")
    print()
    print("Restart Claude Desktop to apply changes.")
    return True

def uninstall():
    config_path = get_claude_config_path()

    if not config_path.exists():
        print("Claude Desktop config not found")
        return False

    try:
        with open(config_path, "r", encoding="utf-8") as f:
            config = json.load(f)
    except (json.JSONDecodeError, FileNotFoundError):
        print("Could not read config")
        return False

    if "mcpServers" in config and "ida-re-assistant" in config["mcpServers"]:
        del config["mcpServers"]["ida-re-assistant"]

        with open(config_path, "w", encoding="utf-8") as f:
            json.dump(config, f, indent=2)

        print("Successfully uninstalled IDA RE Assistant MCP server")
        print("Restart Claude Desktop to apply changes.")
        return True
    else:
        print("IDA RE Assistant not found in config")
        return False

def install_sse_instructions():
    """Show instructions for SSE-based MCP clients"""
    server_path = get_server_path().parent / "ida_sse_server.py"

    print("=" * 60)
    print("SSE MCP Server Setup (ChatGPT, Gemini, etc.)")
    print("=" * 60)
    print()
    print("The SSE server allows any MCP client to connect via HTTP.")
    print()
    print("1. Start the SSE server:")
    print(f"   python {server_path}")
    print()
    print("2. Or use the setup menu:")
    print("   setup.bat -> [4] Start Multi-LLM MCP Server")
    print()
    print("3. In IDA, press Ctrl+Shift+M to start HTTP server")
    print()
    print("4. Configure your MCP client:")
    print("   - MCP Server URL: http://127.0.0.1:13121/sse")
    print("   - Transport: SSE (Server-Sent Events)")
    print()
    print("Supported clients:")
    print("  - ChatGPT with MCP plugin")
    print("  - Google Gemini with MCP support")
    print("  - Any custom MCP client")
    print()

    # Check if dependencies are installed
    try:
        import mcp
        import starlette
        import uvicorn
        print("✓ SSE dependencies already installed")
    except ImportError:
        print("⚠ SSE dependencies not installed!")
        print("  Install with: pip install -r mcp-server/requirements_sse.txt")
    print()

def create_generic_mcp_config():
    """Create a generic MCP config file for reference"""
    script_dir = Path(__file__).parent.resolve()
    config_path = script_dir / "mcp_client_config.json"

    config = {
        "comment": "Generic MCP client configuration for IDA RE Assistant",
        "stdio_transport": {
            "description": "For Claude Desktop and other stdio-based clients",
            "command": str(get_python_path()),
            "args": [str(get_server_path())]
        },
        "sse_transport": {
            "description": "For ChatGPT, Gemini, and HTTP-based clients",
            "url": "http://127.0.0.1:13121/sse",
            "start_server": f"python {get_server_path().parent / 'ida_sse_server.py'}"
        },
        "ida_connection": {
            "description": "IDA HTTP server (start with Ctrl+Shift+M in IDA)",
            "url": "http://127.0.0.1:13120"
        }
    }

    with open(config_path, "w", encoding="utf-8") as f:
        json.dump(config, f, indent=2)

    print(f"Generic config created: {config_path}")
    print("Use this as a reference for configuring your MCP client")
    print()
    return config_path

def show_menu():
    """Show interactive menu for choosing client type"""
    print("=" * 60)
    print("IDA RE Assistant - MCP Installation")
    print("=" * 60)
    print()
    print("Choose your AI client:")
    print()
    print("[1] Claude Desktop (stdio transport)")
    print("[2] ChatGPT / Gemini (SSE transport)")
    print("[3] Other MCP client (show config)")
    print("[4] Uninstall from Claude Desktop")
    print("[Q] Quit")
    print()

    choice = input("Choose option: ").strip().upper()
    return choice

def main():
    # Check for command-line arguments
    if len(sys.argv) > 1:
        if sys.argv[1] == "--uninstall":
            uninstall()
            return
        elif sys.argv[1] == "--claude":
            install()
            return
        elif sys.argv[1] == "--sse":
            install_sse_instructions()
            return
        elif sys.argv[1] == "--help":
            print("Usage:")
            print("  install_mcp.py              # Interactive menu")
            print("  install_mcp.py --claude     # Install for Claude Desktop")
            print("  install_mcp.py --sse        # Show SSE instructions")
            print("  install_mcp.py --uninstall  # Uninstall from Claude Desktop")
            return

    # Interactive menu
    while True:
        choice = show_menu()

        if choice == "1":
            print()
            if install():
                print()
                input("Press Enter to continue...")
            break
        elif choice == "2":
            print()
            install_sse_instructions()
            print()
            input("Press Enter to continue...")
            break
        elif choice == "3":
            print()
            config_path = create_generic_mcp_config()
            print("Configuration options:")
            print()
            print("stdio transport:")
            print(f"  Command: {get_python_path()}")
            print(f"  Args: {get_server_path()}")
            print()
            print("SSE transport:")
            print("  URL: http://127.0.0.1:13121/sse")
            print(f"  Server: {get_server_path().parent / 'ida_sse_server.py'}")
            print()
            input("Press Enter to continue...")
            break
        elif choice == "4":
            print()
            uninstall()
            print()
            input("Press Enter to continue...")
            break
        elif choice == "Q":
            break
        else:
            print("Invalid choice, try again...")
            print()

if __name__ == "__main__":
    main()
