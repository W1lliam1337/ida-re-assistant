# MCP Setup Guide

Complete guide for setting up IDA RE Assistant with various MCP clients.

## What is MCP?

**MCP (Model Context Protocol)** is Anthropic's protocol that allows AI models to interact with external tools and data sources. IDA RE Assistant implements MCP to let AI assistants directly control IDA Pro.

## Supported AI Clients

| Client | Transport | Setup Command |
|--------|-----------|---------------|
| **Claude Desktop** | stdio | `mcp-server/install_mcp.bat` → [1] |
| **ChatGPT** (with MCP plugin) | SSE | `mcp-server/install_mcp.bat` → [2] |
| **Google Gemini** (with MCP) | SSE | `mcp-server/install_mcp.bat` → [2] |
| **Custom MCP client** | stdio or SSE | `mcp-server/install_mcp.bat` → [3] |
| **ImGui Client** (standalone) | Direct API | No MCP needed |

## Quick Setup

### Option 1: Claude Desktop (Recommended for Claude users)

```bash
# 1. Run installer
mcp-server/install_mcp.bat

# 2. Choose [1] Claude Desktop
# 3. Restart Claude Desktop
# 4. In IDA, press Ctrl+Shift+M
# 5. Ask Claude: "What functions are in this binary?"
```

**How it works:**
- Configures `claude_desktop_config.json` automatically
- Uses **stdio transport** (stdin/stdout communication)
- Claude Desktop launches MCP server as subprocess
- MCP server connects to IDA HTTP server (port 13120)

### Option 2: ChatGPT / Gemini (SSE Transport)

```bash
# 1. Run installer
mcp-server/install_mcp.bat

# 2. Choose [2] ChatGPT / Gemini
# 3. Follow instructions to start SSE server
# 4. Configure your MCP client:
#    URL: http://127.0.0.1:13121/sse
```

**How it works:**
- Runs **SSE server** on port 13121
- Uses **Server-Sent Events** (HTTP streaming)
- Any MCP client can connect via HTTP
- SSE server connects to IDA HTTP server (port 13120)

### Option 3: Other MCP Client

```bash
# 1. Run installer
mcp-server/install_mcp.bat

# 2. Choose [3] Other MCP client
# 3. Get config template: mcp_client_config.json
# 4. Configure your client using the template
```

## Transport Types Explained

### stdio Transport (Claude Desktop)

```
AI Client (Claude Desktop)
    ↓ (stdio: stdin/stdout)
MCP Server (ida_bridge.py)
    ↓ (HTTP: port 13120)
IDA Pro (ida_mcp_plugin.py)
```

**Pros:**
- Simple setup (one command)
- No extra server to manage
- Works offline

**Cons:**
- Only works with Claude Desktop
- Can't use other AI models

### SSE Transport (Multi-LLM)

```
AI Client (ChatGPT/Gemini/etc.)
    ↓ (HTTP/SSE: port 13121)
SSE Server (ida_sse_server.py)
    ↓ (HTTP: port 13120)
IDA Pro (ida_mcp_plugin.py)
```

**Pros:**
- Works with any MCP client
- Multiple clients can connect
- Easy to debug (HTTP logs)

**Cons:**
- Need to run SSE server
- Requires extra dependencies

## Manual Setup (Advanced)

### For Claude Desktop (Manual)

1. **Find Claude config:**
   - Windows: `%APPDATA%\Claude\claude_desktop_config.json`
   - macOS: `~/Library/Application Support/Claude/claude_desktop_config.json`
   - Linux: `~/.config/Claude/claude_desktop_config.json`

2. **Add MCP server:**
   ```json
   {
     "mcpServers": {
       "ida-re-assistant": {
         "command": "python",
         "args": ["C:\\path\\to\\ida-re-assistant\\mcp-server\\ida_bridge.py"]
       }
     }
   }
   ```

3. **Restart Claude Desktop**

### For SSE Clients (Manual)

1. **Install dependencies:**
   ```bash
   pip install -r mcp-server/requirements_sse.txt
   ```

2. **Start SSE server:**
   ```bash
   python mcp-server/ida_sse_server.py
   ```

3. **Configure MCP client:**
   - Server URL: `http://127.0.0.1:13121/sse`
   - Transport: SSE (Server-Sent Events)

4. **Start IDA plugin:**
   - In IDA, press `Ctrl+Shift+M`

## Available MCP Tools

Once connected, your AI client has access to these tools:

| Tool | Description | Example |
|------|-------------|---------|
| `get_function_pseudocode` | Decompile a function | "Show me the pseudocode for sub_401000" |
| `get_function_assembly` | Get assembly code | "Show assembly for 0x401000" |
| `get_function_xrefs` | Get cross-references | "What calls this function?" |
| `analyze_function` | Full analysis (pseudo + asm + xrefs) | "Analyze the main function" |
| `list_functions` | List all functions | "What functions are in this binary?" |
| `get_current_function` | Get function at cursor | "Analyze the current function" |
| `rename_function` | Rename a function | "Rename this to parseConfig" |
| `add_comment` | Add comment at address | "Add comment at 0x401234" |
| `get_database_info` | Get binary info (path, MD5) | "What file is this?" |
| `rename_local_variable` | Rename variables | "Rename v3 to buffer_size" |
| `set_variable_type` | Set variable type | "Set buffer type to char*" |
| `add_function_comment` | Add decompiler comment | "Add comment: validates user input" |
| `get_function_local_variables` | List all local variables | "Show me all variables in this function" |

## Usage Examples

### Claude Desktop

```
You: "What does the function at 0x401000 do?"

Claude: Let me analyze that function.
[Uses get_function_pseudocode tool]

This function appears to be a string parser that:
1. Allocates a buffer
2. Copies input data
3. Validates the format
4. Returns parsed result
```

### ChatGPT (via SSE)

```
You: "Find all functions that call malloc"

ChatGPT: I'll search for malloc references.
[Uses list_functions and get_xrefs tools]

Found 5 functions that call malloc:
- sub_401234: Allocates 1024 bytes
- parseConfig: Allocates variable size
- ...
```

## Troubleshooting

### "Cannot connect to IDA"

**Symptoms:** MCP tools fail with connection errors

**Solution:**
1. Make sure IDA Pro is running
2. Press `Ctrl+Shift+M` in IDA
3. Check IDA console for: `[IDA-MCP] HTTP server started on http://127.0.0.1:13120`
4. Check firewall not blocking port 13120

### "SSE server won't start"

**Symptoms:** `ModuleNotFoundError` or import errors

**Solution:**
```bash
pip install -r mcp-server/requirements_sse.txt
```

### "Claude Desktop doesn't see MCP server"

**Symptoms:** No tools available in Claude Desktop

**Solution:**
1. Check config path is correct
2. Verify `claude_desktop_config.json` syntax (valid JSON)
3. Restart Claude Desktop completely
4. Check Python path in config matches your Python installation

### "Variable rename failed"

**Symptoms:** Rename tool returns error

**Solution:**
- Use IDA 9.0 for best compatibility (8.3+ supported)
- Make sure Hex-Rays decompiler is available
- Variable name must be valid C identifier

## Port Reference

| Port | Service | Started By |
|------|---------|------------|
| 13120 | IDA HTTP Server | IDA plugin (`Ctrl+Shift+M`) |
| 13121 | SSE MCP Server | `ida_sse_server.py` or `setup.bat` [4] |

## Configuration Files

| File | Purpose | Location |
|------|---------|----------|
| `claude_desktop_config.json` | Claude Desktop MCP config | `%APPDATA%\Claude\` (Win) |
| `mcp_client_config.json` | Generated config template | Project root (after setup) |
| `mcp_client_config.example.json` | Example config reference | Project root |

## Best Practices

1. **Start IDA first** before connecting MCP clients
2. **Press Ctrl+Shift+M** in IDA to start HTTP server
3. **For SSE:** Start the SSE server before connecting clients
4. **For stdio:** Just restart Claude Desktop after config change
5. **Check logs** in IDA console for connection status

## Alternative: Standalone Client (No MCP)

Don't want to use MCP? Use the **standalone ImGui client** instead:

```bash
setup.bat
# [1] Build Client
# [2] Install Plugin
# Launch ida_re_assistant.exe
# Enter API key in Settings
```

**Advantages:**
- No MCP configuration needed
- Works with all LLM providers (Claude, OpenAI, Gemini)
- Better UI with syntax highlighting and diff viewer
- History and bookmarks
- Custom prompts

---

**For more help, see:**
- [README.md](README.md) - Full documentation
- [QUICK_START.md](QUICK_START.md) - Quick start guide
- [mcp_client_config.example.json](mcp_client_config.example.json) - Config reference
