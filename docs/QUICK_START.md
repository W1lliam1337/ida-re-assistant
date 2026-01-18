# Quick Start Guide

Get up and running with IDA RE Assistant in 5 minutes!

> **See it in action:** Check out [screenshots](README.md#screenshots) in the main README.

## Prerequisites

- **IDA Pro 8.3+** (IDA 9.0 recommended)
- **Windows** (macOS/Linux support coming soon)
- **Visual Studio 2019+** with C++ (for building client)
- **Python 3.8+** (for IDA plugin and MCP servers)

## Step 1: Run Setup (2 minutes)

Open **x64 Native Tools Command Prompt for VS** and run:

```bash
cd path\to\ida-re-assistant
setup.bat
```

**Interactive menu:**
```
[1] Build ImGui Client       <- Choose this first
[2] Install IDA Plugin        <- Then this
[3] Setup Claude Desktop
[4] Start Multi-LLM MCP Server
[5] Check Release Readiness
[Q] Quit
```

1. **Choose [1]** to build the client
   - Downloads dependencies via vcpkg
   - Builds ImGui client
   - Output: `imgui-client\build\bin\Release\ida_re_assistant.exe`

2. **Choose [2]** to install IDA plugin
   - Auto-detects IDA installation
   - Copies plugin to plugins folder
   - Alternative: Use Plugin menu in client GUI

## Step 2: Configure API Key (1 minute)

1. Get an API key from your LLM provider:
   - **Claude**: https://console.anthropic.com/
   - **OpenAI**: https://platform.openai.com/api-keys
   - **Gemini**: https://makersuite.google.com/app/apikey

2. In the client:
   - Click **Settings** (or press `Ctrl+,`)
   - Select your LLM provider
   - Paste your API key
   - Click **Save**

## Step 3: Start Analyzing! (1 minute)

1. **Start IDA Pro** and open your binary
2. Press **Ctrl+Shift+M** in IDA (starts HTTP server)
   - Look for: `[IDA-MCP] HTTP server started on http://127.0.0.1:13120`
3. **Launch the client** if not already running
4. Client should auto-connect to IDA
5. **Navigate** to a function in IDA
6. In the client, click **"Load Current Function"**
7. **Chat** with AI about your function or click analysis buttons!

## Example Workflow

```
1. Load function in IDA
2. In client: "Load Current Function"
3. Ask: "What does this function do?"
4. AI analyzes pseudocode and explains
5. Try: "AI Improve Pseudocode" button
6. Review changes in Diff Viewer
7. Continue reversing with AI assistance!
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+M` | Start IDA HTTP server (in IDA) |
| `Ctrl+L` | Load current function |
| `Ctrl+H` | Show history |
| `Ctrl+B` | Show bookmarks |
| `Ctrl+,` | Settings |

## Troubleshooting

### "Build failed"
- Make sure you're in **x64 Native Tools Command Prompt for VS**
- Try: `rmdir /s /q imgui-client\build` then run `setup.bat` again

### "Cannot connect to IDA"
- Make sure IDA is running
- Press `Ctrl+Shift+M` in IDA
- Check firewall isn't blocking port 13120
- Look for `[IDA-MCP] HTTP server started` in IDA console

### "Hex-Rays decompiler not available"
- You need Hex-Rays license for pseudocode features
- Assembly and xrefs work without it

### "Variable rename failed"
- Use IDA 9.0 for best compatibility
- IDA 8.3+ is supported but may have issues

## Usage Modes

### Mode 1: Standalone Client (Recommended)
✅ What you just set up!
✅ Best for daily use
✅ All LLM providers supported

### Mode 2: Claude Desktop (MCP stdio)
For Claude Desktop users:
```bash
mcp-server/install_mcp.bat
# Choose [1] Claude Desktop
# Restart Claude Desktop
# Press Ctrl+Shift+M in IDA
# Ask Claude about your binary!
```

### Mode 3: ChatGPT / Gemini (MCP via SSE)
For ChatGPT/Gemini with MCP:
```bash
mcp-server/install_mcp.bat
# Choose [2] ChatGPT / Gemini
# Follow the instructions shown
# Press Ctrl+Shift+M in IDA
# Connect MCP client to http://127.0.0.1:13121/sse
```

### Mode 4: Other MCP Client
For custom MCP clients:
```bash
mcp-server/install_mcp.bat
# Choose [3] Other MCP client
# Get config template in mcp_client_config.json
# Configure your client using the template
```

## Next Steps

- Read [README.md](README.md) for full documentation
- Check out **Custom Prompts** for reusable analysis templates
- Try **AI Improve Pseudocode** for auto-renaming variables
- Export analysis to Markdown for reports
- Join discussions for tips and tricks

## Common Tasks

**Rename a function:**
```
Chat: "This looks like a config parser, rename it"
AI will suggest name and apply it to IDA
```

**Understand complex logic:**
```
Load function → Ask: "Explain the algorithm step by step"
```

**Find vulnerabilities:**
```
Load function → Ask: "Are there any security issues here?"
```

**Improve readability:**
```
Click "AI Improve Pseudocode"
→ AI renames variables
→ Adds helpful comments
→ Shows diff viewer
```

## Support

- Issues: GitHub Issues
- Questions: GitHub Discussions
- Full docs: [README.md](README.md)

---

**Happy reversing! 🔍**
