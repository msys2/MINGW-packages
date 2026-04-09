# CMake module to support delay loading of optional DLLs and separating optdepends in MSYS2.
#
# SPDX-FileCopyrightText: 2026 Mikhail Titov <mlt@gmx.us>
# SPDX-License-Identifier: BSD-3-Clause

function(target_delay_load TARGET REGEX)
    set(DELAY_LIBS) # reset for each target
    if(ARGN)
        set(PROP ${ARGN})
    else()
        set(PROP LINK_LIBRARIES)
    endif()
    message(STATUS "Processing ${TARGET} for deps in ${PROP} matching ${REGEX}")
    get_target_property(CURRENT_LIBS ${TARGET} ${PROP})
    if (NOT CURRENT_LIBS)
        message(VERBOSE "  No ${PROP} libraries found for target ${TARGET}")
        return()
    endif()
    foreach(lib IN LISTS CURRENT_LIBS)
        if(TARGET ${lib})
            message(VERBOSE "  keeping target ${lib} as is for target ${TARGET}")
            list(APPEND DELAY_LIBS "${lib}")
            get_target_property(real "${lib}" ALIASED_TARGET)
            if(real)
                set(lib "${real}")
            endif()
            target_delay_load(${lib} "${REGEX}" INTERFACE_LINK_LIBRARIES) # transient dependencies
            target_delay_load(${lib} "${REGEX}" IMPORTED_IMPLIB_RELEASE)
        else() # plain libs
            get_filename_component(libname "${lib}" NAME_WE) # chop .dll.a
            if("${libname}" MATCHES "${REGEX}" AND NOT "${libname}" MATCHES "-delay$")
                message("  found ${libname} for target ${TARGET}")
                execute_process(
                    COMMAND dlltool -I "${lib}"
                    OUTPUT_VARIABLE dllname
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                )
                if(CMAKE_C_COMPILER_ID MATCHES "Clang")
                    list(APPEND DELAY_LIBS "${lib}")
                    list(APPEND DELAY_OPTS "LINKER:/delayload:${dllname}")
                else()
                    set(dllpath "$ENV{MINGW_PREFIX}/bin/${dllname}")
                    execute_process(
                        COMMAND gendef - "${dllpath}"
                        OUTPUT_FILE "${PROJECT_BINARY_DIR}/${libname}.def"
                        ERROR_QUIET
                    )
                    execute_process(
                        COMMAND dlltool -k -d "${libname}.def" -y "${libname}-delay.a"
                        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                    )
                    list(APPEND DELAY_LIBS "${PROJECT_BINARY_DIR}/${libname}-delay.a")
                endif()
                message(VERBOSE "Delay load libs: ${DELAY_LIBS}")
            else()
                message(VERBOSE "  keeping ${libname} as is for target ${TARGET}")
                list(APPEND DELAY_LIBS "${lib}")
            endif()
        endif()
    endforeach()
    message(VERBOSE "Final delay load libs for ${TARGET}: ${DELAY_LIBS}")
    if(NOT ARGN) # top level target the function was called for originally
        list(APPEND DELAY_LIBS "$ENV{MINGW_PREFIX}/lib/dl_reaper.o;delayimp;ntdll")
        list(REMOVE_DUPLICATES DELAY_OPTS)
        target_link_options(${TARGET} PRIVATE ${DELAY_OPTS})
    endif()
    set_property(TARGET ${TARGET} PROPERTY ${PROP} "${DELAY_LIBS}")
    return(PROPAGATE DELAY_OPTS)
endfunction()
