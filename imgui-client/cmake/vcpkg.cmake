include_guard()

# vcpkg auto-bootstrap - downloads and installs vcpkg + packages
# Must be included BEFORE project()

set(VCPKG_REPO "https://github.com/microsoft/vcpkg.git" CACHE STRING "" FORCE)
set(VCPKG_TAG "2024.12.16" CACHE STRING "" FORCE)

get_filename_component(_src_dir "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
set(VCPKG_DIR "${_src_dir}/vcpkg")

if(WIN32)
    set(VCPKG_EXE "${VCPKG_DIR}/vcpkg.exe")
    set(VCPKG_BOOTSTRAP "${VCPKG_DIR}/bootstrap-vcpkg.bat")
    set(VCPKG_TRIPLET "x64-windows-static-md")
    set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_SOURCE_DIR}/triplets")
else()
    set(VCPKG_EXE "${VCPKG_DIR}/vcpkg")
    set(VCPKG_BOOTSTRAP "${VCPKG_DIR}/bootstrap-vcpkg.sh")
    set(VCPKG_TRIPLET "x64-linux")
endif()

# Clone vcpkg
if(NOT EXISTS "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")
    message(STATUS "[vcpkg] Cloning to ${VCPKG_DIR}...")
    execute_process(
        COMMAND git clone --depth 1 --branch ${VCPKG_TAG} ${VCPKG_REPO} "${VCPKG_DIR}"
        RESULT_VARIABLE _result
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "[vcpkg] git clone failed")
    endif()
endif()

# Bootstrap vcpkg
if(NOT EXISTS "${VCPKG_EXE}")
    message(STATUS "[vcpkg] Bootstrapping...")
    execute_process(
        COMMAND "${VCPKG_BOOTSTRAP}" -disableMetrics
        WORKING_DIRECTORY "${VCPKG_DIR}"
        RESULT_VARIABLE _result
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "[vcpkg] bootstrap failed")
    endif()
endif()

# Install packages from vcpkg.json
set(_installed "${VCPKG_DIR}/installed/${VCPKG_TRIPLET}")
if(EXISTS "${_src_dir}/vcpkg.json" AND NOT EXISTS "${_installed}/include/openssl")
    message(STATUS "[vcpkg] Installing packages...")
    set(_vcpkg_cmd "${VCPKG_EXE}" install
        --triplet=${VCPKG_TRIPLET}
        --x-install-root=${VCPKG_DIR}/installed)
    if(DEFINED VCPKG_OVERLAY_TRIPLETS)
        list(APPEND _vcpkg_cmd --overlay-triplets=${VCPKG_OVERLAY_TRIPLETS})
    endif()
    execute_process(
        COMMAND ${_vcpkg_cmd}
        WORKING_DIRECTORY "${_src_dir}"
        RESULT_VARIABLE _result
    )
    if(NOT _result EQUAL 0)
        message(FATAL_ERROR "[vcpkg] install failed")
    endif()
endif()

message(STATUS "[vcpkg] Ready: ${VCPKG_DIR}")

# Set CMake toolchain
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" FORCE)
set(VCPKG_TARGET_TRIPLET "${VCPKG_TRIPLET}" CACHE STRING "" FORCE)
if(DEFINED VCPKG_OVERLAY_TRIPLETS)
    set(VCPKG_OVERLAY_TRIPLETS "${VCPKG_OVERLAY_TRIPLETS}" CACHE STRING "" FORCE)
endif()
