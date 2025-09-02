# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright 2016 Blender Foundation. All rights reserved.

# Libraries configuration for MINGW

add_definitions(-DWIN32 -D_WIN32_WINNT=0x603 -DFREE_WINDOWS)

# Support restoring this value once pre-compiled libraries have been handled.
set(WITH_STATIC_LIBS_INIT ${WITH_STATIC_LIBS})

# Wrapper to prefer static libraries
macro(find_package_wrapper)
  if(WITH_STATIC_LIBS)
    find_package_static(${ARGV})
  else()
    find_package(${ARGV})
  endif()
endmacro()

# ----------------------------------------------------------------------------
# Precompiled Libraries
#

find_package_wrapper(JPEG REQUIRED)
find_package_wrapper(PNG REQUIRED)
find_package_wrapper(ZLIB REQUIRED)
find_package_wrapper(Zstd REQUIRED)
find_package_wrapper(Epoxy REQUIRED)

# XXX Linking errors with debian static tiff :/
# find_package_wrapper(TIFF REQUIRED)
find_package(TIFF)
# CMake 3.28.1 defines this, it doesn't seem to be used, hide by default in the UI.
# NOTE(@ideasman42): this doesn't seem to be important,
# on my system it's not-found even when the TIFF library is.

if(WITH_VULKAN_BACKEND)
  find_package_wrapper(Vulkan REQUIRED)
  find_package_wrapper(ShaderC REQUIRED)
  find_package_wrapper(glslang CONFIG REQUIRED)
  list(APPEND SHADERC_LIBRARIES glslang::glslang glslang::SPIRV)
endif()

function(check_freetype_for_brotli)
  if((DEFINED HAVE_BROTLI AND HAVE_BROTLI) AND
     (DEFINED HAVE_BROTLI_INC AND ("${HAVE_BROTLI_INC}" STREQUAL "${FREETYPE_INCLUDE_DIRS}")))
    # Pass, the includes didn't change, use the cached value.
  else()
    unset(HAVE_BROTLI CACHE)
    include(CheckSymbolExists)
    set(CMAKE_REQUIRED_INCLUDES ${FREETYPE_INCLUDE_DIRS})
    check_symbol_exists(FT_CONFIG_OPTION_USE_BROTLI "freetype/config/ftconfig.h" HAVE_BROTLI)
    unset(CMAKE_REQUIRED_INCLUDES)
    if(NOT HAVE_BROTLI)
      unset(HAVE_BROTLI CACHE)
      message(FATAL_ERROR "Freetype needs to be compiled with brotli support!")
    endif()
    set(HAVE_BROTLI_INC "${FREETYPE_INCLUDE_DIRS}" CACHE INTERNAL "")
  endif()
endfunction()

find_package_wrapper(Freetype REQUIRED)
find_package_wrapper(Brotli REQUIRED)
check_freetype_for_brotli()

if(WITH_PYTHON)
  # This could be used, see: D14954 for details.
  # `find_package(PythonLibs)`
  find_package(PythonLibsUnix REQUIRED)
else()
  # Python executable is needed as part of the build-process,
  # note that building without Python is quite unusual.
  find_program(PYTHON_EXECUTABLE "python3")
endif()

if(WITH_IMAGE_OPENEXR)
  find_package_wrapper(OpenEXR)  # our own module
  set_and_warn_library_found("OpenEXR" OPENEXR_FOUND WITH_IMAGE_OPENEXR)
endif()
add_bundled_libraries(openexr/lib)
add_bundled_libraries(imath/lib)

if(WITH_IMAGE_OPENJPEG)
  find_package_wrapper(OpenJPEG)
  set_and_warn_library_found("OpenJPEG" OPENJPEG_FOUND WITH_IMAGE_OPENJPEG)
endif()

if(WITH_OPENAL)
  find_package_wrapper(OpenAL)
  set_and_warn_library_found("OpenAL" OPENAL_FOUND WITH_OPENAL)
endif()

if(WITH_SDL)
  find_package_wrapper(SDL2)
  if(SDL2_FOUND)
    # Use same names for both versions of SDL until we move to 2.x.
    set(SDL_INCLUDE_DIR "${SDL2_INCLUDE_DIR}")
    set(SDL_LIBRARY "${SDL2_LIBRARY}")
    set(SDL_FOUND "${SDL2_FOUND}")
  else()
    find_package_wrapper(SDL)
  endif()
  mark_as_advanced(
    SDL_INCLUDE_DIR
    SDL_LIBRARY
  )
  # unset(SDLMAIN_LIBRARY CACHE)
  set_and_warn_library_found("SDL" SDL_FOUND WITH_SDL)
endif()

# Codecs
if(WITH_CODEC_SNDFILE)
  find_package_wrapper(SndFile)
  set_and_warn_library_found("libsndfile" SNDFILE_FOUND WITH_CODEC_SNDFILE)
endif()

if(WITH_CODEC_FFMPEG)
  if(FFMPEG)
    # Old cache variable used for root dir, convert to new standard.
    set(FFMPEG_ROOT_DIR ${FFMPEG})
  endif()
  find_package(FFmpeg)
  set_and_warn_library_found("FFmpeg" FFMPEG_FOUND WITH_CODEC_FFMPEG)
endif()

if(WITH_FFTW3)
  find_package_wrapper(Fftw3)
  set_and_warn_library_found("fftw3" FFTW3_FOUND WITH_FFTW3)
endif()

if(WITH_OPENCOLLADA)
  find_package_wrapper(OpenCOLLADA)
  if(OPENCOLLADA_FOUND)

    find_package_wrapper(XML2)
  else()
    set_and_warn_library_found("OpenCollada" OPENCOLLADA_FOUND WITH_OPENCOLLADA)
  endif()
endif()

if(WITH_MEM_JEMALLOC)
  find_package_wrapper(JeMalloc)
  set_and_warn_library_found("JeMalloc" JEMALLOC_FOUND WITH_MEM_JEMALLOC)
endif()

if(WITH_INPUT_NDOF)
  find_package_wrapper(Spacenav)
  set_and_warn_library_found("SpaceNav" SPACENAV_FOUND WITH_INPUT_NDOF)

  if(SPACENAV_FOUND)
    # use generic names within blenders buildsystem.
    set(NDOF_INCLUDE_DIRS ${SPACENAV_INCLUDE_DIRS})
    set(NDOF_LIBRARIES ${SPACENAV_LIBRARIES})
  endif()
endif()

if(WITH_CYCLES AND WITH_CYCLES_OSL)
  find_package_wrapper(OSL 1.13.4)
  set_and_warn_library_found("OSL" OSL_FOUND WITH_CYCLES_OSL)
  
  if(OSL_FOUND)
    if(${OSL_LIBRARY_VERSION_MAJOR} EQUAL "1" AND ${OSL_LIBRARY_VERSION_MINOR} LESS "6")
      # Note: --whole-archive is needed to force loading of all symbols in liboslexec,
      # otherwise LLVM is missing the osl_allocate_closure_component function
      set(OSL_LIBRARIES
        ${OSL_OSLCOMP_LIBRARY}
        -Wl,--whole-archive ${OSL_OSLEXEC_LIBRARY}
        -Wl,--no-whole-archive ${OSL_OSLQUERY_LIBRARY}
      )
    endif()
  endif()
endif()
add_bundled_libraries(osl/lib)

if(WITH_CYCLES)
  set(CYCLES_LEVEL_ZERO ${LIBDIR}/level-zero CACHE PATH "Path to Level Zero installation")
  mark_as_advanced(CYCLES_LEVEL_ZERO)
  if(EXISTS ${CYCLES_LEVEL_ZERO} AND NOT LEVEL_ZERO_ROOT_DIR)
    set(LEVEL_ZERO_ROOT_DIR ${CYCLES_LEVEL_ZERO})
  endif()

  set(CYCLES_SYCL ${LIBDIR}/dpcpp CACHE PATH "Path to oneAPI DPC++ compiler")
  mark_as_advanced(CYCLES_SYCL)
  if(EXISTS ${CYCLES_SYCL} AND NOT SYCL_ROOT_DIR)
    set(SYCL_ROOT_DIR ${CYCLES_SYCL})
  endif()
endif()

if(WITH_OPENVDB)
  find_package(OpenVDB)
  set_and_warn_library_found("OpenVDB" OPENVDB_FOUND WITH_OPENVDB)
endif()
add_bundled_libraries(openvdb/lib)

if(WITH_NANOVDB)
  find_package_wrapper(NanoVDB)
  set_and_warn_library_found("NanoVDB" NANOVDB_FOUND WITH_NANOVDB)
endif()

if(WITH_CPU_SIMD AND SUPPORT_NEON_BUILD)
  find_package_wrapper(sse2neon)
endif()

if(WITH_ALEMBIC)
  find_package_wrapper(Alembic)
  set_and_warn_library_found("Alembic" ALEMBIC_FOUND WITH_ALEMBIC)
endif()

if(WITH_USD)
  find_package_wrapper(USD)
  set_and_warn_library_found("USD" USD_FOUND WITH_USD)
  set_and_warn_library_found("Hydra" USD_FOUND WITH_HYDRA)
endif()
add_bundled_libraries(usd/lib)

if(WITH_MATERIALX)
  find_package_wrapper(MaterialX)
  set_and_warn_library_found("MaterialX" MaterialX_FOUND WITH_MATERIALX)
endif()
add_bundled_libraries(materialx/lib)

# With Blender 4.4 libraries there is no more Boost. But Linux distros may have
# older versions of libs like USD with a header dependency on Boost, so can't
# remove this entirely yet.
if(WITH_BOOST)
  if(DEFINED LIBDIR AND NOT EXISTS "${LIBDIR}/boost")
    set(WITH_BOOST OFF)
    set(BOOST_LIBRARIES)
    set(BOOST_PYTHON_LIBRARIES)
    set(BOOST_INCLUDE_DIR)
  endif()
endif()

if(WITH_BOOST)
  # uses in build instructions to override include and library variables
  if(NOT BOOST_CUSTOM)
    if(WITH_STATIC_LIBS)
      set(Boost_USE_STATIC_LIBS OFF)
    endif()
    set(Boost_USE_MULTITHREADED ON)
    set(__boost_packages)
    if(WITH_USD AND USD_PYTHON_SUPPORT)
      list(APPEND __boost_packages python${PYTHON_VERSION_NO_DOTS})
    endif()
    set(Boost_NO_WARN_NEW_VERSIONS ON)
    find_package(Boost 1.48 COMPONENTS ${__boost_packages})
    if(NOT Boost_FOUND)
      # try to find non-multithreaded if -mt not found, this flag
      # doesn't matter for us, it has nothing to do with thread
      # safety, but keep it to not disturb build setups
      set(Boost_USE_MULTITHREADED OFF)
      find_package(Boost 1.48 COMPONENTS ${__boost_packages})
    endif()
    unset(__boost_packages)
    mark_as_advanced(Boost_DIR)  # why doesn't boost do this?
    mark_as_advanced(Boost_INCLUDE_DIR)  # why doesn't boost do this?
  endif()

  # Boost Python is the only library Blender directly depends on, though USD headers.
  if(WITH_USD AND USD_PYTHON_SUPPORT)
    set(BOOST_PYTHON_LIBRARIES ${Boost_PYTHON${PYTHON_VERSION_NO_DOTS}_LIBRARY})
  endif()
  set(BOOST_INCLUDE_DIR ${Boost_INCLUDE_DIRS})
  set(BOOST_LIBPATH ${Boost_LIBRARY_DIRS})
  set(BOOST_DEFINITIONS "-DBOOST_ALL_NO_LIB")
endif()
add_bundled_libraries(boost/lib)

if(WITH_PUGIXML)
  find_package_wrapper(PugiXML)
  set_and_warn_library_found("PugiXML" PUGIXML_FOUND WITH_PUGIXML)
endif()

if(WITH_IMAGE_WEBP)
  set(WEBP_ROOT_DIR ${LIBDIR}/webp)
  find_package_wrapper(WebP)
  set_and_warn_library_found("WebP" WEBP_FOUND WITH_IMAGE_WEBP)
endif()

find_package_wrapper(OpenImageIO REQUIRED)
add_bundled_libraries(openimageio/lib)

if(WITH_OPENCOLORIO)
  find_package_wrapper(OpenColorIO 2.0.0)

  set(OPENCOLORIO_DEFINITIONS "")
  set_and_warn_library_found("OpenColorIO" OPENCOLORIO_FOUND WITH_OPENCOLORIO)
endif()
add_bundled_libraries(opencolorio/lib)

if(WITH_CYCLES AND WITH_CYCLES_EMBREE)
  find_package(Embree 4.0.0 REQUIRED)
endif()
add_bundled_libraries(embree/lib)

if(WITH_OPENIMAGEDENOISE)
  find_package_wrapper(OpenImageDenoise)
  set_and_warn_library_found("OpenImageDenoise" OPENIMAGEDENOISE_FOUND WITH_OPENIMAGEDENOISE)
  add_bundled_libraries(openimagedenoise/lib)
endif()

if(WITH_LLVM)
  find_package_wrapper(LLVM)
  set_and_warn_library_found("LLVM" LLVM_FOUND WITH_LLVM)

  if(LLVM_FOUND)
    if(WITH_CLANG)
      find_package_wrapper(Clang)
      set_and_warn_library_found("Clang" CLANG_FOUND WITH_CLANG)
    endif()

    # Symbol conflicts with same UTF library used by OpenCollada
    if(DEFINED LIBDIR)
      if(WITH_OPENCOLLADA AND (${LLVM_VERSION} VERSION_LESS "4.0.0"))
        list(REMOVE_ITEM OPENCOLLADA_LIBRARIES ${OPENCOLLADA_UTF_LIBRARY})
      endif()
    endif()
  endif()
endif()

if(WITH_OPENSUBDIV)
  find_package(OpenSubdiv)

  set(OPENSUBDIV_LIBRARIES ${OPENSUBDIV_LIBRARIES})
  set(OPENSUBDIV_LIBPATH)  # TODO, remove and reference the absolute path everywhere

  set_and_warn_library_found("OpenSubdiv" OPENSUBDIV_FOUND WITH_OPENSUBDIV)
endif()
add_bundled_libraries(opensubdiv/lib)

if(WITH_TBB)
  find_package_wrapper(TBB 2021.13.0)
  if(TBB_FOUND)
    get_target_property(TBB_LIBRARIES TBB::tbb LOCATION)
    get_target_property(TBB_INCLUDE_DIRS TBB::tbb INTERFACE_INCLUDE_DIRECTORIES)
  endif()
  set_and_warn_library_found("TBB" TBB_FOUND WITH_TBB)
endif()
add_bundled_libraries(tbb/lib)

if(WITH_XR_OPENXR)
  find_package(XR_OpenXR_SDK)
  set_and_warn_library_found("OpenXR-SDK" XR_OPENXR_SDK_FOUND WITH_XR_OPENXR)
endif()

if(WITH_GMP)
  find_package_wrapper(GMP)
  set_and_warn_library_found("GMP" GMP_FOUND WITH_GMP)
endif()

if(WITH_POTRACE)
  find_package_wrapper(Potrace)
  set_and_warn_library_found("Potrace" POTRACE_FOUND WITH_POTRACE)
endif()

if(WITH_HARU)
  find_package_wrapper(Haru)
  set_and_warn_library_found("Haru" HARU_FOUND WITH_HARU)
endif()

if(WITH_MANIFOLD)
  # This isn't a common system library, so disable if it's not found.
  find_package(manifold REQUIRED)
  if(TARGET manifold::manifold)
    set(MANIFOLD_FOUND TRUE)
  endif()
  set_and_warn_library_found("MANIFOLD" MANIFOLD_FOUND WITH_MANIFOLD)
endif()

if(WITH_CYCLES AND WITH_CYCLES_PATH_GUIDING)
  find_package_wrapper(openpgl)
  if(openpgl_FOUND)
    get_target_property(OPENPGL_LIBRARIES openpgl::openpgl LOCATION)
    get_target_property(OPENPGL_INCLUDE_DIR openpgl::openpgl INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "Found OpenPGL: ${OPENPGL_LIBRARIES}")
  else()
    set(WITH_CYCLES_PATH_GUIDING OFF)
    message(STATUS "OpenPGL not found, disabling WITH_CYCLES_PATH_GUIDING")
  endif()
endif()

# ----------------------------------------------------------------------------
# Build and Link Flags

list(APPEND PLATFORM_LINKLIBS
  ws2_32 vfw32 winmm kernel32 user32 gdi32 comdlg32 comctl32 version
  advapi32 shfolder shell32 ole32 oleaut32 uuid psapi dbghelp shlwapi
  pathcch shcore dwmapi crypt32
)

if(WITH_INPUT_IME)
  list(APPEND PLATFORM_LINKLIBS imm32)
endif()

find_package(Threads REQUIRED)
# `FindThreads` documentation notes that this may be empty
# with the system libraries provide threading functionality.
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PLATFORM_LINKLIBS ${CMAKE_THREAD_LIBS_INIT})
  # used by other platforms
  set(PTHREADS_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
endif()

# ----------------------------------------------------------------------------
# System Libraries
#
# Keep last, so indirectly linked libraries don't override our own pre-compiled libs.

if(WITH_SYSTEM_FREETYPE)
  find_package_wrapper(Freetype)
  if(NOT FREETYPE_FOUND)
    message(FATAL_ERROR "Failed finding system FreeType version!")
  endif()
  check_freetype_for_brotli()
  # Quiet warning as this variable will be used after `FREETYPE_LIBRARIES`.
  set(BROTLI_LIBRARIES "")
endif()

if(WITH_LZO AND WITH_SYSTEM_LZO)
  find_package_wrapper(LZO)
  if(NOT LZO_FOUND)
    message(FATAL_ERROR "Failed finding system LZO version!")
  endif()
endif()

if(WITH_SYSTEM_EIGEN3)
  find_package_wrapper(Eigen3)
  if(NOT EIGEN3_FOUND)
    message(FATAL_ERROR "Failed finding system Eigen3 version!")
  endif()
endif()

# Jack is intended to use the system library.
if(WITH_JACK)
  find_package_wrapper(Jack)
  set_and_warn_library_found("JACK" JACK_FOUND WITH_JACK)
endif()

# Pulse is intended to use the system library.
if(WITH_PULSEAUDIO)
  find_package_wrapper(Pulse)
  set_and_warn_library_found("PulseAudio" PULSE_FOUND WITH_PULSEAUDIO)
endif()

# Audio IO
if(WITH_SYSTEM_AUDASPACE)
  find_package_wrapper(Audaspace)
  set(AUDASPACE_FOUND ${AUDASPACE_FOUND} AND ${AUDASPACE_C_FOUND})
  set_and_warn_library_found("External Audaspace" AUDASPACE_FOUND WITH_SYSTEM_AUDASPACE)
endif()

# ----------------------------------------------------------------------------
# Compilers

# GNU Compiler
if(CMAKE_COMPILER_IS_GNUCC)
  # ffp-contract=off:
  # Automatically turned on when building with "-march=native". This is
  # explicitly turned off here as it will make floating point math give a bit
  # different results. This will lead to automated test failures. So disable
  # this until we support it.
  set(PLATFORM_CFLAGS "-pipe -fPIC -funsigned-char -fno-strict-aliasing -ffp-contract=off")

  # `maybe-uninitialized` is unreliable in release builds, but fine in debug builds.
  set(GCC_EXTRA_FLAGS_RELEASE "-Wno-maybe-uninitialized")
  set(CMAKE_C_FLAGS_RELEASE          "${GCC_EXTRA_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELEASE}")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO   "${GCC_EXTRA_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_CXX_FLAGS_RELEASE        "${GCC_EXTRA_FLAGS_RELEASE} ${CMAKE_CXX_FLAGS_RELEASE}")
  set(CMAKE_C_FLAGS_DEBUG            "-DXXH_NO_INLINE_HINTS=1")
  string(PREPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO "${GCC_EXTRA_FLAGS_RELEASE} ")
  unset(GCC_EXTRA_FLAGS_RELEASE)
  
  if(WITH_LINKER_LLD AND _IS_LINKER_DEFAULT)
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} -fuse-ld=lld -Wl,--version
      ERROR_QUIET OUTPUT_VARIABLE LD_VERSION)
    if("${LD_VERSION}" MATCHES "LLD")
      string(APPEND CMAKE_EXE_LINKER_FLAGS    " -fuse-ld=lld")
      string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fuse-ld=lld")
      string(APPEND CMAKE_MODULE_LINKER_FLAGS " -fuse-ld=lld")
      set(_IS_LINKER_DEFAULT OFF)
    else()
      message(STATUS "LLD linker isn't available, using the default system linker.")
    endif()
    unset(LD_VERSION)
  endif()

# CLang is the same as GCC for now.
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
  set(PLATFORM_CFLAGS "-pipe -fPIC -funsigned-char -fno-strict-aliasing -ffp-contract=off -D_USE_MATH_DEFINES")
  
  if(WITH_LINKER_LLD AND _IS_LINKER_DEFAULT)
    find_program(LLD_BIN "ld.lld")
    mark_as_advanced(LLD_BIN)
    if(NOT LLD_BIN)
      message(STATUS "The \"ld.lld\" binary could not be found, using system linker.")
      set(WITH_LINKER_LLD OFF)
    else()
      if(CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0)
        string(APPEND CMAKE_EXE_LINKER_FLAGS    " --ld-path=\"${LLD_BIN}\"")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " --ld-path=\"${LLD_BIN}\"")
        string(APPEND CMAKE_MODULE_LINKER_FLAGS " --ld-path=\"${LLD_BIN}\"")
      else()
        string(APPEND CMAKE_EXE_LINKER_FLAGS    " -fuse-ld=\"${LLD_BIN}\"")
        string(APPEND CMAKE_SHARED_LINKER_FLAGS " -fuse-ld=\"${LLD_BIN}\"")
        string(APPEND CMAKE_MODULE_LINKER_FLAGS " -fuse-ld=\"${LLD_BIN}\"")
      endif()
      set(_IS_LINKER_DEFAULT OFF)
    endif()
    unset(LLD_BIN)
  endif()
endif()

string(APPEND CMAKE_C_FLAGS_DEBUG " -D_DEBUG")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

# Don't use position independent executable for portable install since file
# browsers can't properly detect blender as an executable then. Still enabled
# for non-portable installs as typically used by Linux distributions.
if(WITH_INSTALL_PORTABLE)
  string(APPEND CMAKE_EXE_LINKER_FLAGS " -no-pie")
endif()

if(WITH_COMPILER_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  mark_as_advanced(CCACHE_PROGRAM)
  if(CCACHE_PROGRAM)
    # Makefiles and ninja
    set(CMAKE_C_COMPILER_LAUNCHER   "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "" FORCE)
    mark_as_advanced(
      CMAKE_C_COMPILER_LAUNCHER
      CMAKE_CXX_COMPILER_LAUNCHER
    )
  else()
    message(WARNING "Ccache NOT found, disabling WITH_COMPILER_CCACHE")
    set(WITH_COMPILER_CCACHE OFF)
  endif()
endif()

# Always link with libatomic if available, as it is required for data types
# which don't have intrinsics.
function(configure_atomic_lib_if_needed)
  set(_source
      "#include <atomic>
      #include <cstdint>
      int main(int argc, char **argv) {
        std::atomic<uint64_t> uint64; uint64++;
        return 0;
      }"
  )

  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_LIBRARIES atomic)
  check_cxx_source_compiles("${_source}" ATOMIC_OPS_WITH_LIBATOMIC)
  unset(CMAKE_REQUIRED_LIBRARIES)

  if(ATOMIC_OPS_WITH_LIBATOMIC)
    set(PLATFORM_LINKFLAGS "${PLATFORM_LINKFLAGS} -latomic" PARENT_SCOPE)
  endif()
endfunction()

configure_atomic_lib_if_needed()
