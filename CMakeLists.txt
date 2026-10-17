cmake_minimum_required(VERSION 3.16)
project(touhou075 C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Upstream game logic (Goan114/th075 for SteinsGateON, th075_web/cpp/game) +
# the shared GLES renderer, and the Switch host in src/ (upstream
# th075_web/cpp/sdl with the browser runtime replaced by src/main.cpp).
file(GLOB GAME_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/game/*.cpp)
file(GLOB HOST_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM HOST_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)
list(APPEND HOST_SOURCES portable/sdl/Renderer.cpp)

# CP932 -> UTF-16 map for the on-demand glyph renderer.
set(CP932_TABLE ${CMAKE_BINARY_DIR}/generated/cp932_table.inc)
add_custom_command(OUTPUT ${CP932_TABLE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/generated
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/gen_cp932.py ${CP932_TABLE}
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/gen_cp932.py
    COMMENT "Generating CP932 table" VERBATIM)

# Game + host objects, shared by the game and the host tests.
add_library(th075_core OBJECT ${GAME_SOURCES} ${HOST_SOURCES} ${CP932_TABLE})
target_include_directories(th075_core PUBLIC src ${CMAKE_BINARY_DIR}/generated)

# Same code-generation contract as the upstream web build: no FMA
# contraction (bit-exact float order with the original), no strict aliasing,
# no exceptions/RTTI. GCC defaults to -ffp-contract=fast on AArch64, so this
# flag is load-bearing for replay/RNG determinism. -fsigned-char: plain char
# is signed on x86 and wasm32, unsigned on AArch64; keep the original's.
target_compile_options(th075_core PUBLIC
    -O2 -ffp-contract=off -fno-strict-aliasing -fsigned-char
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions -fno-rtti>
    $<$<CXX_COMPILER_ID:GNU>:-Wno-class-memaccess> -Wno-misleading-indentation -Wno-parentheses)
target_compile_definitions(th075_core PUBLIC TH_NATIVE_PLATFORM=1 TH075_DEVELOPMENT_HARNESS=0)

add_executable(touhou075 src/main.cpp)
target_link_libraries(touhou075 PRIVATE th075_core)

if(NINTENDO_SWITCH)
    # ---- Nintendo Switch (devkitA64; configure with the devkitPro Switch
    # toolchain file, which defines NINTENDO_SWITCH) ----
    # Layout/naming follows the Saekaze Touhou ports: touhou075.nro +
    # sdmc:/switch/touhou/touhou075/ (th075.dat, th075b.dat, th075bgm.dat,
    # msgothic.ttc).
    if(NOT DEFINED ENV{DEVKITPRO})
        set(ENV{DEVKITPRO} "/opt/devkitpro")
    endif()
    set(TH_DEVKITPRO "$ENV{DEVKITPRO}")
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg")
        message(FATAL_ERROR "platform/switch/icon.jpg (256x256 JPEG) not found.")
    endif()
    target_compile_definitions(th075_core PUBLIC __SWITCH__)
    target_include_directories(th075_core PUBLIC
        ${TH_DEVKITPRO}/portlibs/switch/include
        ${TH_DEVKITPRO}/portlibs/switch/include/SDL2
        ${TH_DEVKITPRO}/portlibs/switch/include/freetype2
        ${TH_DEVKITPRO}/libnx/include)
    target_link_directories(touhou075 PRIVATE
        ${TH_DEVKITPRO}/portlibs/switch/lib
        ${TH_DEVKITPRO}/libnx/lib)
    # Static link order matters: dependents first, dependencies after.
    target_link_libraries(touhou075 PRIVATE
        SDL2 EGL glapi drm_nouveau GLESv2 freetype harfbuzz png bz2 z nx m)
    set_target_properties(touhou075 PROPERTIES OUTPUT_NAME "touhou075")

    # Default (all languages): romanised title. The original Japanese title is
    # patched into the Japanese slot only - same as the other Saekaze ports.
    nx_generate_nacp(OUTPUT "${CMAKE_BINARY_DIR}/touhou075-base.nacp"
        NAME "Touhou 7.5: Immaterial and Missing Power"
        AUTHOR "Twilight Frontier / Team Shanghai Alice"
        VERSION "1.11-r1")
    add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/touhou075.nacp"
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
                "${CMAKE_BINARY_DIR}/touhou075-base.nacp" "${CMAKE_BINARY_DIR}/touhou075.nacp"
                --lang Japanese
                --name "東方萃夢想　～ Immaterial and Missing Power."
                --author "黄昏フロンティア・上海アリス幻樂団"
                --fill-empty-name "Touhou 7.5: Immaterial and Missing Power"
                --fill-empty-author "Twilight Frontier / Team Shanghai Alice"
        DEPENDS "${CMAKE_BINARY_DIR}/touhou075-base.nacp"
                "${CMAKE_SOURCE_DIR}/scripts/nacp_lang.py"
        COMMENT "Patching Japanese title into NACP"
        VERBATIM)
    nx_create_nro(touhou075
        ICON "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg"
        NACP "${CMAKE_BINARY_DIR}/touhou075.nacp")
else()
    # ---- Linux test build (same sources, SDL2 + GLES3 desktop) ----
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    pkg_check_modules(FT REQUIRED freetype2)
    target_include_directories(th075_core PUBLIC ${SDL2_INCLUDE_DIRS} ${FT_INCLUDE_DIRS})
    target_link_libraries(touhou075 PRIVATE ${SDL2_LIBRARIES} ${FT_LIBRARIES} GLESv2 m)

    # Host tests: ./build/th075_host_tests (run under xvfb-run for the renderer)
    add_executable(th075_host_tests tests/test_host.cpp)
    target_link_libraries(th075_host_tests PRIVATE th075_core ${SDL2_LIBRARIES} ${FT_LIBRARIES} GLESv2 m)

endif()
