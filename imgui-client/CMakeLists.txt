cmake_minimum_required(VERSION 3.20)

# Auto-bootstrap vcpkg (must be before project())
include(cmake/vcpkg.cmake)

project(ida_re_assistant LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# dependencies paths
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/libs/imgui)
set(HTTPLIB_DIR ${CMAKE_SOURCE_DIR}/libs/cpp-httplib)

# vcpkg packages
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Fix OpenSSL library paths for custom triplet
set(OPENSSL_CRYPTO_LIBRARY "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libcrypto.lib")
set(OPENSSL_SSL_LIBRARY "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/lib/libssl.lib")
set(OPENSSL_LIBRARIES ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} crypt32 ws2_32)

# ImGui library
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Precompiled header macro for MSVC (requires explicit #include "vendor.hpp" in each .cpp)
macro(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
    if(MSVC)
        get_filename_component(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
        set(PrecompiledBinary "${CMAKE_CURRENT_BINARY_DIR}/${PrecompiledBasename}.pch")
        set(Sources ${${SourcesVar}})

        # Set /Yc flag for PCH source file (create PCH)
        set_source_files_properties(${PrecompiledSource}
            PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\""
                       OBJECT_OUTPUTS "${PrecompiledBinary}")

        # Set /Yu flag for all other source files (use PCH)
        set_source_files_properties(${Sources}
            PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\""
                       OBJECT_DEPENDS "${PrecompiledBinary}")

        # Add PCH source to sources list
        list(APPEND ${SourcesVar} ${PrecompiledSource})
    endif()
endmacro()

# Source files for main executable
set(IDA_RE_SOURCES
    src/main.cpp
    src/api/mcp_client.cpp
    src/api/llm_api.cpp
    src/ui/ui.cpp
    src/core/installer.cpp
    src/utils/syntax_highlighter.cpp
    src/utils/analysis_history.cpp
)

# Add precompiled header (MSVC only, requires explicit #include "vendor.hpp")
ADD_MSVC_PRECOMPILED_HEADER("vendor.hpp" "src/vendor.cpp" IDA_RE_SOURCES)

# main executable
add_executable(ida_re_assistant WIN32 ${IDA_RE_SOURCES})

target_include_directories(ida_re_assistant PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${HTTPLIB_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_link_libraries(ida_re_assistant PRIVATE
    imgui
    d3d11
    d3dcompiler
    dxgi
    ${OPENSSL_LIBRARIES}
    nlohmann_json::nlohmann_json
)

target_compile_definitions(ida_re_assistant PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
    _CRT_SECURE_NO_WARNINGS
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)
