# CMakeLists.txt for AutoHotkey v2 -- MinGW/GCC build
# Written for the MSYS2 packaging effort.
cmake_minimum_required(VERSION 3.20)

# Extract version from git or use fallback
execute_process(
  COMMAND git describe --tags --match "v*"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_DESCRIBE
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
  RESULT_VARIABLE GIT_RESULT
)

if(GIT_RESULT EQUAL 0 AND GIT_DESCRIBE MATCHES "^v([0-9]+\\.[0-9]+\\.[0-9]+)")
  set(AHK_VERSION "${CMAKE_MATCH_1}")
  string(REGEX REPLACE "^v" "" RAW_AHK_VERSION "${GIT_DESCRIBE}")
  string(REGEX REPLACE "-([0-9]+)-g" "+\\1+" RAW_AHK_VERSION "${RAW_AHK_VERSION}")
  string(REGEX REPLACE "-dirty$" "+dirty" RAW_AHK_VERSION "${RAW_AHK_VERSION}")
else()
  set(AHK_VERSION "2.0.23")
  set(RAW_AHK_VERSION "2.0.23")
endif()

project(AutoHotkey VERSION ${AHK_VERSION} LANGUAGES C CXX ASM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# --- Bundled PCRE (static library) ---
set(PCRE_SOURCES
  source/lib_pcre/pcre/pcre16_chartables.c
  source/lib_pcre/pcre/pcre16_compile.c
  source/lib_pcre/pcre/pcre16_config.c
  source/lib_pcre/pcre/pcre16_exec.c
  source/lib_pcre/pcre/pcre16_fullinfo.c
  source/lib_pcre/pcre/pcre16_get.c
  source/lib_pcre/pcre/pcre16_globals.c
  source/lib_pcre/pcre/pcre16_jit_compile.c
  source/lib_pcre/pcre/pcre16_newline.c
  source/lib_pcre/pcre/pcre16_ord2utf16.c
  source/lib_pcre/pcre/pcre16_refcount.c
  source/lib_pcre/pcre/pcre16_string_utils.c
  source/lib_pcre/pcre/pcre16_study.c
  source/lib_pcre/pcre/pcre16_tables.c
  source/lib_pcre/pcre/pcre16_ucd.c
  source/lib_pcre/pcre/pcre16_valid_utf16.c
  source/lib_pcre/pcre/pcre16_version.c
  source/lib_pcre/pcre/pcre16_xclass.c
)

add_library(lib_pcre STATIC ${PCRE_SOURCES})
target_compile_definitions(lib_pcre PRIVATE
  HAVE_CONFIG_H
  WIN32
  _LIB
  PCRE_STATIC
)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_compile_definitions(lib_pcre PRIVATE _WIN64)
endif()
target_include_directories(lib_pcre PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/lib_pcre/pcre
)

# --- AutoHotkey main executable ---
set(AHK_SOURCES
  source/ahkversion.cpp
  source/application.cpp
  source/AutoHotkey.cpp
  source/MdFunc.cpp
  source/clipboard.cpp
  source/Debugger.cpp
  source/error.cpp
  source/globaldata.cpp
  source/hook.cpp
  source/hotkey.cpp
  source/input_object.cpp
  source/keyboard_mouse.cpp
  source/lib/CCallback.cpp
  source/lib/DllCall.cpp
  source/lib/drive.cpp
  source/lib/env.cpp
  source/lib/file.cpp
  source/lib/Gui.ListView.cpp
  source/lib/Gui.StatusBar.cpp
  source/lib/Gui.TreeView.cpp
  source/lib/input.cpp
  source/lib/InputBox.cpp
  source/lib/interop.cpp
  source/lib/math.cpp
  source/lib/pixel.cpp
  source/lib/process.cpp
  source/lib/regex.cpp
  source/lib/sound.cpp
  source/lib/string.cpp
  source/lib/vars.cpp
  source/lib/wait.cpp
  source/lib/win.cpp
  source/os_version.cpp
  source/script.cpp
  source/script2.cpp
  source/script_autoit.cpp
  source/script_com.cpp
  source/script_expression.cpp
  source/script_gui.cpp
  source/script_menu.cpp
  source/script_object.cpp
  source/script_object_bif.cpp
  source/script_registry.cpp
  source/SimpleHeap.cpp
  source/StringConv.cpp
  source/TextIO.cpp
  source/util.cpp
  source/var.cpp
  source/window.cpp
  source/WinGroup.cpp
)

# x64 assembly (generated from upstream MASM by masm2gas.awk)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  enable_language(ASM)
  list(APPEND AHK_SOURCES
    x64call.S
    x64stub.S
  )
endif()

# Resource file
set(AHK_RC source/resources/AutoHotkey.rc)

add_executable(AutoHotkey WIN32 ${AHK_SOURCES} ${AHK_RC})

target_compile_definitions(AutoHotkey PRIVATE
  WIN32
  _WINDOWS
  UNICODE
  _UNICODE
  NDEBUG
  ENABLE_DLLCALL
  ENABLE_REGISTERCALLBACK
  CONFIG_DEBUGGER
  "RAW_AHK_VERSION=\"${RAW_AHK_VERSION}\""
  _WIN32_WINNT=0x0600
  _WIN32_IE=0x0700
  PCRE_STATIC
)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  target_compile_definitions(AutoHotkey PRIVATE _WIN64)
endif()

target_include_directories(AutoHotkey PRIVATE
  ${CMAKE_SOURCE_DIR}/source
  ${CMAKE_SOURCE_DIR}/source/lib_pcre/pcre
)

# Generate a version header for the resource compiler (avoids windres quote-escaping issues)
file(WRITE ${CMAKE_BINARY_DIR}/ahk_version_rc.h
  "#define RAW_AHK_VERSION \"${RAW_AHK_VERSION}\"\n"
  "#define AHK_VERSION_N ${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0\n"
)
set_source_files_properties(${AHK_RC} PROPERTIES COMPILE_FLAGS
  "--preprocessor-arg=-include --preprocessor-arg=${CMAKE_BINARY_DIR}/ahk_version_rc.h"
)
set_source_files_properties(${AHK_RC} PROPERTIES COMPILE_DEFINITIONS "")

target_link_libraries(AutoHotkey PRIVATE
  lib_pcre
  wsock32
  ws2_32
  winmm
  version
  comctl32
  psapi
  wininet
  shlwapi
  uxtheme
  dwmapi
  ole32
  oleaut32
  uuid
  gdi32
  comdlg32
  shell32
  user32
  kernel32
  advapi32
  mpr
)

# MinGW-specific flags
if(MINGW)
  target_compile_options(AutoHotkey PRIVATE
    -O2
    -fomit-frame-pointer
    -fno-threadsafe-statics
    -fpermissive
    -Wno-narrowing
    -Wno-unused-result
    -Wno-deprecated-declarations
    -Wno-switch
    -Wno-unknown-pragmas
    -Wno-parentheses
  )
  target_link_options(AutoHotkey PRIVATE
    -municode
    -Wl,--stack,4194304
    -Wl,--disable-dynamicbase
    -Wl,--disable-nxcompat
  )
  # The manifest is embedded via the .rc resource file, not the linker.
endif()

set_target_properties(AutoHotkey PROPERTIES
  OUTPUT_NAME "AutoHotkey64"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

install(TARGETS AutoHotkey RUNTIME DESTINATION bin)
install(FILES license.txt DESTINATION share/licenses/autohotkey)
