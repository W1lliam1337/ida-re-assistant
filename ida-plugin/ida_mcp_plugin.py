"""
IDA Pro MCP Plugin - Connects IDA to Claude via MCP protocol
Runs HTTP server inside IDA, external bridge handles stdio<->HTTP translation
"""

import idaapi
import ida_hexrays
import ida_funcs
import ida_bytes
import ida_name
import ida_segment
import ida_lines
import idautils
import idc

try:
    import ida_typeinf
    HAS_TYPEINF = True
except ImportError:
    HAS_TYPEINF = False
import json
import threading
import socket
import http.server
import socketserver
from typing import Optional
from urllib.parse import urlparse, parse_qs


class IDADataProvider:
    """Extracts data from IDA database"""

    @staticmethod
    def get_function_by_address(addr_str: str) -> Optional[ida_funcs.func_t]:
        """Get function object by address string"""
        try:
            if addr_str.startswith("0x") or addr_str.startswith("0X"):
                ea = int(addr_str, 16)
            else:
                ea = int(addr_str)
            return ida_funcs.get_func(ea)
        except ValueError:
            return None

    @staticmethod
    def get_function_pseudocode(addr_str: str) -> dict:
        """Decompile function and return pseudocode"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        if not ida_hexrays.init_hexrays_plugin():
            return {"error": "Hex-Rays decompiler not available"}

        try:
            cfunc = ida_hexrays.decompile(func)
            if cfunc:
                pseudocode = str(cfunc)
                return {
                    "address": f"0x{func.start_ea:X}",
                    "name": ida_name.get_name(func.start_ea) or f"sub_{func.start_ea:X}",
                    "pseudocode": pseudocode
                }
            return {"error": "Decompilation failed"}
        except ida_hexrays.DecompilationFailure as e:
            return {"error": f"Decompilation failed: {str(e)}"}
        except Exception as e:
            return {"error": str(e)}

    @staticmethod
    def get_function_assembly(addr_str: str) -> dict:
        """Get disassembly for function"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        lines = []
        ea = func.start_ea
        while ea < func.end_ea:
            disasm = idc.GetDisasm(ea)
            if disasm:
                lines.append(f"{ea:08X}  {disasm}")
            ea = idc.next_head(ea, func.end_ea)

        return {
            "address": f"0x{func.start_ea:X}",
            "name": ida_name.get_name(func.start_ea) or f"sub_{func.start_ea:X}",
            "assembly": "\n".join(lines)
        }

    @staticmethod
    def get_function_xrefs(addr_str: str) -> dict:
        """Get cross-references for function"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        xrefs_to = []  # Functions that call this function
        xrefs_from = []  # Functions/APIs this function calls

        # Get references TO this function
        for xref in idautils.XrefsTo(func.start_ea, 0):
            caller_name = ida_name.get_name(xref.frm)
            if not caller_name:
                caller_func = ida_funcs.get_func(xref.frm)
                if caller_func:
                    caller_name = ida_name.get_name(caller_func.start_ea) or f"sub_{caller_func.start_ea:X}"
                else:
                    caller_name = f"0x{xref.frm:X}"
            xrefs_to.append(caller_name)

        # Get references FROM this function (calls made by this function)
        for head in idautils.Heads(func.start_ea, func.end_ea):
            for xref in idautils.XrefsFrom(head, 0):
                if xref.type in [idautils.ida_xref.fl_CN, idautils.ida_xref.fl_CF]:
                    callee_name = ida_name.get_name(xref.to)
                    if not callee_name:
                        callee_name = f"0x{xref.to:X}"
                    if callee_name not in xrefs_from:
                        xrefs_from.append(callee_name)

        return {
            "address": f"0x{func.start_ea:X}",
            "name": ida_name.get_name(func.start_ea) or f"sub_{func.start_ea:X}",
            "calls_to": list(set(xrefs_to)),
            "calls_from": xrefs_from
        }

    @staticmethod
    def analyze_function(addr_str: str) -> dict:
        """Get complete function analysis"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        result = {
            "address": f"0x{func.start_ea:X}",
            "name": ida_name.get_name(func.start_ea) or f"sub_{func.start_ea:X}",
            "size": func.end_ea - func.start_ea,
        }

        # Get pseudocode
        pseudo_data = IDADataProvider.get_function_pseudocode(addr_str)
        if "pseudocode" in pseudo_data:
            result["pseudocode"] = pseudo_data["pseudocode"]
        else:
            result["pseudocode_error"] = pseudo_data.get("error", "Unknown error")

        # Get assembly
        asm_data = IDADataProvider.get_function_assembly(addr_str)
        if "assembly" in asm_data:
            result["assembly"] = asm_data["assembly"]

        # Get xrefs
        xref_data = IDADataProvider.get_function_xrefs(addr_str)
        result["xrefs"] = {
            "calls_to": xref_data.get("calls_to", []),
            "calls_from": xref_data.get("calls_from", [])
        }

        return result

    @staticmethod
    def list_functions(limit: int = 100) -> dict:
        """List all functions in the binary"""
        funcs = []
        for idx, ea in enumerate(idautils.Functions()):
            if limit and idx >= limit:
                break
            name = ida_name.get_name(ea) or f"sub_{ea:X}"
            func = ida_funcs.get_func(ea)
            size = func.end_ea - func.start_ea if func else 0
            funcs.append({
                "address": f"0x{ea:X}",
                "name": name,
                "size": size
            })

        return {
            "functions": funcs,
            "total": len(list(idautils.Functions())),
            "returned": len(funcs)
        }

    @staticmethod
    def get_current_function() -> dict:
        """Get function at current cursor position"""
        ea = idc.get_screen_ea()
        func = ida_funcs.get_func(ea)
        if not func:
            return {"error": "No function at current cursor position"}
        return IDADataProvider.analyze_function(f"0x{func.start_ea:X}")

    @staticmethod
    def rename_function(addr_str: str, new_name: str) -> dict:
        """Rename a function"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        if ida_name.set_name(func.start_ea, new_name, ida_name.SN_CHECK):
            return {
                "success": True,
                "address": f"0x{func.start_ea:X}",
                "new_name": new_name
            }
        return {"error": "Failed to rename function"}

    @staticmethod
    def add_comment(addr_str: str, comment: str, is_repeatable: bool = False) -> dict:
        """Add comment at address"""
        try:
            if addr_str.startswith("0x") or addr_str.startswith("0X"):
                ea = int(addr_str, 16)
            else:
                ea = int(addr_str)

            if is_repeatable:
                idc.set_cmt(ea, comment, 1)
            else:
                idc.set_cmt(ea, comment, 0)

            return {
                "success": True,
                "address": f"0x{ea:X}",
                "comment": comment
            }
        except Exception as e:
            return {"error": str(e)}

    @staticmethod
    def rename_local_variable(addr_str: str, old_name: str, new_name: str) -> dict:
        """Rename local variable in decompiled function using ida_hexrays.rename_lvar"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        if not ida_hexrays.init_hexrays_plugin():
            return {"error": "Hex-Rays decompiler not available"}

        try:
            # Use ida_hexrays.rename_lvar - the correct API
            success = ida_hexrays.rename_lvar(func.start_ea, old_name, new_name)

            if success:
                # Refresh decompiler view
                cfunc = ida_hexrays.decompile(func)
                if cfunc:
                    cfunc.refresh_func_ctext()

                return {
                    "success": True,
                    "address": f"0x{func.start_ea:X}",
                    "old_name": old_name,
                    "new_name": new_name
                }
            else:
                return {
                    "error": f"Failed to rename '{old_name}' to '{new_name}'. Variable may not exist or name is invalid."
                }
        except Exception as e:
            return {"error": str(e)}

    @staticmethod
    def set_variable_type(addr_str: str, var_name: str, type_str: str) -> dict:
        """Set type for local variable"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        if not ida_hexrays.init_hexrays_plugin():
            return {"error": "Hex-Rays decompiler not available"}

        try:
            cfunc = ida_hexrays.decompile(func)
            if not cfunc:
                return {"error": "Decompilation failed"}

            # Find variable
            lvars = cfunc.get_lvars()
            found = False
            for lvar in lvars:
                if lvar.name == var_name:
                    found = True
                    # Parse and set type
                    tif = ida_typeinf.tinfo_t()
                    if ida_typeinf.parse_decl(tif, None, type_str, ida_typeinf.PT_SIL):
                        # Create lvar_saved_info_t with new type
                        lsi = ida_hexrays.lvar_saved_info_t()
                        lsi.ll = lvar
                        lsi.name = lvar.name
                        lsi.type = tif
                        lsi.size = lvar.width

                        # Apply using modify_user_lvars
                        if ida_hexrays.modify_user_lvars(func.start_ea, lsi):
                            ida_hexrays.mark_cfunc_dirty(func.start_ea)

                            return {
                                "success": True,
                                "address": f"0x{func.start_ea:X}",
                                "variable": var_name,
                                "type": type_str
                            }
                        return {"error": f"modify_user_lvars failed for '{var_name}'"}
                    return {"error": f"Invalid type: {type_str}"}

            if not found:
                return {"error": f"Variable '{var_name}' not found"}
        except Exception as e:
            return {"error": str(e)}

    @staticmethod
    def add_function_comment(addr_str: str, comment: str, line_number: Optional[int] = None) -> dict:
        """Add comment to function (both disassembly and decompiler views) - Working code from ida-pro-mcp"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        try:
            ea = func.start_ea

            # Set disassembly comment first
            if not idaapi.set_cmt(ea, comment, False):
                return {"error": f"Failed to set disassembly comment at {hex(ea)}"}

            # Try to set decompiler comment
            if not ida_hexrays.init_hexrays_plugin():
                return {"success": True, "address": f"0x{ea:X}", "note": "Decompiler not available"}

            try:
                cfunc = ida_hexrays.decompile(ea)
                if not cfunc:
                    return {"success": True, "address": f"0x{ea:X}", "note": "Decompilation failed"}

                # If it's the function entry, set function comment
                if ea == cfunc.entry_ea:
                    idc.set_func_cmt(ea, comment, True)
                    cfunc.refresh_func_ctext()
                    return {"success": True, "address": f"0x{ea:X}"}

                # Try to set comment at specific address in decompiler
                eamap = cfunc.get_eamap()
                if ea not in eamap:
                    return {
                        "success": True,
                        "address": f"0x{ea:X}",
                        "note": "Address not in decompiler eamap"
                    }

                nearest_ea = eamap[ea][0].ea

                # Clean orphan comments first
                if cfunc.has_orphan_cmts():
                    cfunc.del_orphan_cmts()
                    cfunc.save_user_cmts()

                # Try different insertion points (ITP_SEMI, ITP_DO, ITP_ELSE, ITP_COLON)
                tl = idaapi.treeloc_t()
                tl.ea = nearest_ea
                for itp in range(idaapi.ITP_SEMI, idaapi.ITP_COLON):
                    tl.itp = itp
                    cfunc.set_user_cmt(tl, comment)
                    cfunc.save_user_cmts()
                    cfunc.refresh_func_ctext()
                    if not cfunc.has_orphan_cmts():
                        return {"success": True, "address": f"0x{ea:X}"}
                    cfunc.del_orphan_cmts()
                    cfunc.save_user_cmts()

                # If all insertion points failed, still return success (disassembly comment was set)
                return {
                    "success": True,
                    "address": f"0x{ea:X}",
                    "note": "Decompiler comment placement failed (all insertion points created orphans)"
                }

            except Exception as e:
                return {
                    "success": True,
                    "address": f"0x{ea:X}",
                    "note": f"Decompiler comment failed: {str(e)}"
                }

        except Exception as e:
            return {"error": str(e)}

    @staticmethod
    def get_function_local_variables(addr_str: str) -> dict:
        """Get list of all local variables in function"""
        func = IDADataProvider.get_function_by_address(addr_str)
        if not func:
            return {"error": f"No function at {addr_str}"}

        if not ida_hexrays.init_hexrays_plugin():
            return {"error": "Hex-Rays decompiler not available"}

        try:
            cfunc = ida_hexrays.decompile(func)
            if not cfunc:
                return {"error": "Decompilation failed"}

            variables = []
            lvars = cfunc.get_lvars()
            for lvar in lvars:
                var_info = {
                    "name": lvar.name,
                    "type": str(lvar.type()),
                    "is_arg": lvar.is_arg_var,
                    "width": lvar.width
                }
                variables.append(var_info)

            return {
                "address": f"0x{func.start_ea:X}",
                "variables": variables
            }
        except Exception as e:
            return {"error": str(e)}

    @staticmethod
    def get_database_info() -> dict:
        """Get information about current IDA database"""
        import os
        input_file = idaapi.get_input_file_path()
        idb_path = idaapi.get_path(idaapi.PATH_TYPE_IDB)

        # Get input file name with fallback to IDB name
        if input_file:
            input_file_name = os.path.basename(input_file)
        elif idb_path:
            # Fallback: use IDB name without extension
            idb_name = os.path.basename(idb_path)
            input_file_name = os.path.splitext(idb_name)[0]
        else:
            input_file_name = "unknown"

        return {
            "input_file": input_file or "",
            "input_file_name": input_file_name,
            "idb_path": idb_path or "",
            "md5": idaapi.retrieve_input_file_md5().hex() if idaapi.retrieve_input_file_md5() else ""
        }


# Tool definitions for MCP
MCP_TOOLS = [
    {
        "name": "get_function_pseudocode",
        "description": "Get decompiled pseudocode for a function at specified address",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address (e.g., 0x401000)"}
            },
            "required": ["address"]
        }
    },
    {
        "name": "get_function_assembly",
        "description": "Get assembly code for a function at specified address",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"}
            },
            "required": ["address"]
        }
    },
    {
        "name": "get_function_xrefs",
        "description": "Get cross-references (calls to/from) for a function",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"}
            },
            "required": ["address"]
        }
    },
    {
        "name": "analyze_function",
        "description": "Get complete analysis of a function (pseudocode + assembly + xrefs)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"}
            },
            "required": ["address"]
        }
    },
    {
        "name": "list_functions",
        "description": "List all functions in the binary",
        "inputSchema": {
            "type": "object",
            "properties": {
                "limit": {"type": "integer", "description": "Max functions to return"}
            }
        }
    },
    {
        "name": "get_current_function",
        "description": "Get analysis of function at current cursor position",
        "inputSchema": {"type": "object", "properties": {}}
    },
    {
        "name": "rename_function",
        "description": "Rename a function",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"},
                "new_name": {"type": "string", "description": "New function name"}
            },
            "required": ["address", "new_name"]
        }
    },
    {
        "name": "add_comment",
        "description": "Add comment at address",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Address"},
                "comment": {"type": "string", "description": "Comment text"},
                "repeatable": {"type": "boolean", "description": "Repeatable comment"}
            },
            "required": ["address", "comment"]
        }
    },
    {
        "name": "get_database_info",
        "description": "Get information about current IDA database (file name, path, MD5)",
        "inputSchema": {"type": "object", "properties": {}}
    },
    {
        "name": "rename_local_variable",
        "description": "Rename a local variable in a decompiled function",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"},
                "old_name": {"type": "string", "description": "Current variable name"},
                "new_name": {"type": "string", "description": "New variable name"}
            },
            "required": ["address", "old_name", "new_name"]
        }
    },
    {
        "name": "set_variable_type",
        "description": "Set the type of a local variable in a decompiled function",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"},
                "var_name": {"type": "string", "description": "Variable name"},
                "type_str": {"type": "string", "description": "C type declaration (e.g., 'int', 'char*', 'struct foo*')"}
            },
            "required": ["address", "var_name", "type_str"]
        }
    },
    {
        "name": "add_function_comment",
        "description": "Add a comment to a decompiled function (at start or specific line)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"},
                "comment": {"type": "string", "description": "Comment text"},
                "line_number": {"type": "integer", "description": "Optional line number (0-based)"}
            },
            "required": ["address", "comment"]
        }
    },
    {
        "name": "get_function_local_variables",
        "description": "Get list of all local variables in a function with their types",
        "inputSchema": {
            "type": "object",
            "properties": {
                "address": {"type": "string", "description": "Function address"}
            },
            "required": ["address"]
        }
    }
]


def execute_tool(name: str, arguments: dict) -> dict:
    """Execute a tool and return the result"""
    result = [None]

    def execute_on_main_thread():
        try:
            if name == "get_function_pseudocode":
                result[0] = IDADataProvider.get_function_pseudocode(arguments["address"])
            elif name == "get_function_assembly":
                result[0] = IDADataProvider.get_function_assembly(arguments["address"])
            elif name == "get_function_xrefs":
                result[0] = IDADataProvider.get_function_xrefs(arguments["address"])
            elif name == "analyze_function":
                result[0] = IDADataProvider.analyze_function(arguments["address"])
            elif name == "list_functions":
                limit = arguments.get("limit", 100)
                result[0] = IDADataProvider.list_functions(limit)
            elif name == "get_current_function":
                result[0] = IDADataProvider.get_current_function()
            elif name == "rename_function":
                result[0] = IDADataProvider.rename_function(arguments["address"], arguments["new_name"])
            elif name == "add_comment":
                result[0] = IDADataProvider.add_comment(
                    arguments["address"],
                    arguments["comment"],
                    arguments.get("repeatable", False)
                )
            elif name == "get_database_info":
                result[0] = IDADataProvider.get_database_info()
            elif name == "rename_local_variable":
                result[0] = IDADataProvider.rename_local_variable(
                    arguments["address"],
                    arguments["old_name"],
                    arguments["new_name"]
                )
            elif name == "set_variable_type":
                result[0] = IDADataProvider.set_variable_type(
                    arguments["address"],
                    arguments["var_name"],
                    arguments["type_str"]
                )
            elif name == "add_function_comment":
                result[0] = IDADataProvider.add_function_comment(
                    arguments["address"],
                    arguments["comment"],
                    arguments.get("line_number")
                )
            elif name == "get_function_local_variables":
                result[0] = IDADataProvider.get_function_local_variables(arguments["address"])
            else:
                result[0] = {"error": f"Unknown tool: {name}"}
        except Exception as e:
            result[0] = {"error": str(e)}
        return 1

    # Execute on main thread using IDA's execute_sync
    idaapi.execute_sync(execute_on_main_thread, idaapi.MFF_FAST)
    return result[0]


class IDAHTTPHandler(http.server.BaseHTTPRequestHandler):
    """HTTP request handler for IDA MCP bridge"""

    def log_message(self, format, *args):
        print(f"[IDA-MCP] {args[0]}")

    def send_json(self, data: dict, status: int = 200):
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        path = urlparse(self.path).path

        if path == "/health":
            self.send_json({"status": "ok", "server": "ida-mcp"})

        elif path == "/tools":
            self.send_json({"tools": MCP_TOOLS})

        else:
            self.send_json({"error": "Not found"}, 404)

    def do_POST(self):
        path = urlparse(self.path).path

        content_length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_length).decode() if content_length > 0 else "{}"

        try:
            data = json.loads(body)
        except json.JSONDecodeError:
            self.send_json({"error": "Invalid JSON"}, 400)
            return

        if path == "/call":
            tool_name = data.get("name")
            arguments = data.get("arguments", {})

            if not tool_name:
                self.send_json({"error": "Missing tool name"}, 400)
                return

            result = execute_tool(tool_name, arguments)
            self.send_json({"result": result})

        else:
            self.send_json({"error": "Not found"}, 404)


class ThreadedHTTPServer(socketserver.ThreadingMixIn, http.server.HTTPServer):
    """HTTP server that handles each request in a new thread"""
    allow_reuse_address = True
    daemon_threads = True


class HTTPServerThread(threading.Thread):
    """Runs HTTP server in background thread"""

    DEFAULT_PORT = 13120

    def __init__(self, port: int = None):
        super().__init__(daemon=True)
        self.port = port or self.DEFAULT_PORT
        self.server = None
        self.running = False

    def run(self):
        self.running = True

        try:
            self.server = ThreadedHTTPServer(("127.0.0.1", self.port), IDAHTTPHandler)
            print(f"[IDA-MCP] HTTP server started on http://127.0.0.1:{self.port}")
            print(f"[IDA-MCP] Run the bridge to connect Claude Desktop")
            self.server.serve_forever()
        except OSError as e:
            if e.errno == 10048:  # Port already in use
                print(f"[IDA-MCP] Port {self.port} already in use")
            else:
                print(f"[IDA-MCP] Server error: {e}")
        except Exception as e:
            print(f"[IDA-MCP] Server error: {e}")
        finally:
            self.running = False

    def stop(self):
        if self.server:
            self.server.shutdown()
        self.running = False


class IDAMCPPlugin(idaapi.plugin_t):
    """IDA Plugin class"""

    flags = idaapi.PLUGIN_KEEP
    comment = "MCP Server for Claude AI integration"
    help = "Connects IDA to Claude via MCP protocol"
    wanted_name = "IDA MCP Plugin"
    wanted_hotkey = "Ctrl-Shift-M"

    def __init__(self):
        self.server_thread = None

    def init(self):
        print("[IDA-MCP] Plugin loaded")
        print("[IDA-MCP] Press Ctrl+Shift+M to start HTTP server")
        return idaapi.PLUGIN_KEEP

    def run(self, arg):
        if self.server_thread and self.server_thread.running:
            print("[IDA-MCP] Server already running")
            idaapi.info(f"MCP HTTP Server already running on port {self.server_thread.port}")
            return

        self.server_thread = HTTPServerThread()
        self.server_thread.start()

        idaapi.info(
            f"MCP HTTP Server started on port {self.server_thread.port}!\n\n"
            f"Now run the bridge script to connect Claude Desktop:\n"
            f"python ida_bridge.py"
        )

    def term(self):
        if self.server_thread:
            self.server_thread.stop()
            print("[IDA-MCP] Server stopped")


def PLUGIN_ENTRY():
    return IDAMCPPlugin()


# For running as script (not plugin)
if __name__ == "__main__":
    print("[IDA-MCP] Running as script...")
    thread = HTTPServerThread()
    thread.start()
    print(f"[IDA-MCP] Server running on http://127.0.0.1:{thread.port}")
    print("[IDA-MCP] Press Ctrl+C to stop.")
    try:
        while thread.running:
            idaapi.qsleep(1000)
    except KeyboardInterrupt:
        thread.stop()
        print("[IDA-MCP] Server stopped")
