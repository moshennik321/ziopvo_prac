cmake_minimum_required(VERSION 3.20)
project(TrayApp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    # Bundle the MSVC runtime into the binaries so the build artifacts do not
    # depend on redistributable DLLs being present on the target machine.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_executable(TrayApp WIN32
    src/main.cpp
    src/app.rc
    src/rpc_client.cpp
    src/trayapp_rpc_c.c
    src/trayapp_rpc_shared.cpp
)

target_include_directories(TrayApp PRIVATE src)

target_link_libraries(TrayApp PRIVATE
    shell32
    user32
    gdi32
    comctl32
    advapi32
    rpcrt4
)

# UTF-8 source encoding
target_compile_options(TrayApp PRIVATE /utf-8)

# Enable manifest for visual styles
target_link_options(TrayApp PRIVATE
    /MANIFEST:EMBED
    /MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/app.manifest
)

# ---- Windows Service --------------------------------------------------------
add_executable(TrayAppService
    src/backend_client.cpp
    src/service.cpp
    src/trayapp_rpc_s.c
    src/trayapp_rpc_shared.cpp
)

target_compile_options(TrayAppService PRIVATE /utf-8)

target_link_libraries(TrayAppService PRIVATE
    advapi32
    iphlpapi
    winhttp
    wtsapi32
    userenv
    rpcrt4
)
