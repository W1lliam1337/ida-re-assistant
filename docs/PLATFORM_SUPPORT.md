# Platform Support

## Current Status

| Platform | Client | IDA Plugin | MCP Servers | Status |
|----------|--------|------------|-------------|---------|
| Windows  | ✅ Full Support | ✅ Tested | ✅ Full Support | **Supported** |
| macOS    | ⚠️ Needs Update | ✅ Should Work | ✅ Full Support | **Planned** |
| Linux    | ⚠️ Needs Update | ✅ Should Work | ✅ Full Support | **Planned** |

## Windows (Fully Supported)

### Client
- **Graphics Backend**: DirectX 11 (via D3D11)
- **Window System**: Win32 API
- **Build System**: CMake + Visual Studio 2019+
- **Dependencies**: Managed via vcpkg

### IDA Plugin
- **Tested Versions**: IDA Pro 8.3, 9.0
- **Python**: IDA's built-in Python 3
- **Installation**: `%APPDATA%\Hex-Rays\IDA Pro\plugins\`

### MCP Servers
- **stdio transport**: ✅ Works
- **SSE transport**: ✅ Works (requires pip packages)

## macOS (Needs Update)

### What Works Now
- ✅ IDA Plugin (should work without changes)
- ✅ MCP Servers (Python is platform-independent)
- ✅ Build system (CMake is cross-platform)

### What Needs Updates

#### 1. Update `main.cpp` for Metal Backend

Current (Windows DirectX 11):
```cpp
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <d3d11.h>
```

Needs change to (macOS Metal):
```cpp
#ifdef __APPLE__
#include <imgui_impl_osx.h>
#include <imgui_impl_metal.h>
#include <Metal/Metal.h>
#else
// Windows code
#endif
```

#### 2. Update Font Paths

Current:
```cpp
const char* font_path = "C:\\Windows\\Fonts\\Verdana.ttf";
```

Needs:
```cpp
#ifdef __APPLE__
const char* font_path = "/System/Library/Fonts/Helvetica.ttc";
#elif defined(_WIN32)
const char* font_path = "C:\\Windows\\Fonts\\Verdana.ttf";
#else
const char* font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
#endif
```

#### 3. Update CMakeLists.txt

Add Metal framework:
```cmake
if(APPLE)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${METAL_LIBRARY}
        ${METALKIT_LIBRARY}
    )
endif()
```

#### 4. IDA Plugin Installation Path

macOS: `~/.idapro/plugins/`

### Build Instructions (macOS)

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install CMake
brew install cmake

# Build
cd imgui-client
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/bin/ida_re_assistant
```

## Linux (Needs Update)

### What Works Now
- ✅ IDA Plugin (should work without changes)
- ✅ MCP Servers (Python is platform-independent)
- ✅ Build system (CMake is cross-platform)

### What Needs Updates

#### 1. Update `main.cpp` for OpenGL Backend

Needs (Linux OpenGL):
```cpp
#ifdef __linux__
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#else
// Windows code
#endif
```

#### 2. Update Font Paths

Common Linux font paths:
- `/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf`
- `/usr/share/fonts/TTF/DejaVuSans.ttf`
- `~/.fonts/`

#### 3. Update CMakeLists.txt

Add OpenGL dependencies:
```cmake
if(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    find_package(glfw3 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        OpenGL::GL
        glfw
        dl
        pthread
    )
endif()
```

#### 4. IDA Plugin Installation Path

Linux: `~/.idapro/plugins/`

### Build Instructions (Linux)

```bash
# Install dependencies
sudo apt install build-essential cmake libglfw3-dev libgl1-mesa-dev

# Build
cd imgui-client
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/bin/ida_re_assistant
```

## Required Changes Summary

### Files That Need Platform-Specific Code

1. **`imgui-client/src/main.cpp`** (CRITICAL)
   - Add Metal backend for macOS
   - Add OpenGL/GLFW backend for Linux
   - Platform-specific initialization
   - Platform-specific event loop

2. **`imgui-client/CMakeLists.txt`** (IMPORTANT)
   - Add Metal framework linking (macOS)
   - Add OpenGL/GLFW linking (Linux)
   - Platform-specific compiler flags

3. **`build.bat`** → **`build.sh`** (HELPFUL)
   - Create shell script version for Unix systems
   - Handle different package managers (brew/apt)

### Files That Work Cross-Platform (No Changes Needed)

- ✅ All Python code (IDA plugin, MCP servers)
- ✅ Core C++ logic (UI, API clients, config)
- ✅ JSON/HTTP libraries
- ✅ ImGui core library

## Testing Checklist (When Porting)

### macOS
- [ ] Client launches without crash
- [ ] Window renders correctly with Metal
- [ ] Can connect to IDA on localhost
- [ ] All UI elements work
- [ ] Font rendering works
- [ ] Plugin installs to correct directory
- [ ] IDA plugin starts HTTP server
- [ ] MCP stdio works with Claude Desktop
- [ ] MCP SSE server starts

### Linux
- [ ] Client launches without crash
- [ ] Window renders correctly with OpenGL
- [ ] Can connect to IDA on localhost
- [ ] All UI elements work
- [ ] Font rendering works
- [ ] Plugin installs to correct directory
- [ ] IDA plugin starts HTTP server
- [ ] MCP stdio works
- [ ] MCP SSE server starts

## Contribution Opportunities

Want to help add macOS/Linux support? Here's how:

1. **Fork the repository**
2. **Update `main.cpp`** with platform-specific rendering
3. **Update `CMakeLists.txt`** with platform libs
4. **Test on your platform**
5. **Submit pull request**

We'll review and merge platform support contributions!

## Why Windows-Only For Now?

- DirectX 11 was fastest to implement
- Most IDA users are on Windows
- Cross-platform planned but not MVP blocker
- Metal/OpenGL ports are straightforward

## Future Plans

### v1.1 (macOS Support)
- Metal backend for main.cpp
- macOS build script
- Test on macOS 12+
- Update documentation

### v1.2 (Linux Support)
- OpenGL backend for main.cpp
- Linux build script
- Test on Ubuntu 22.04, Arch
- Update documentation

### v2.0 (Full Cross-Platform)
- Unified build system
- Platform detection
- CI/CD for all platforms
- Binary releases for all

## Questions?

- Open an issue on GitHub
- Check discussions for platform-specific topics
- We're happy to guide contributors!

---

**Contributions welcome! Let's make this work everywhere.** 🌍
