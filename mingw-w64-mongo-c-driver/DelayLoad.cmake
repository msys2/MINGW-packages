# Delay load dependency libraries
function(delay_load libs libs_out opts_out)
    foreach(implib IN LISTS libs)
        if (TARGET ${implib})
            get_target_property(real "${implib}" ALIASED_TARGET)
            if(real)
                set(implib "${real}")
            endif()
            string(FIND "${implib}" "::" namespace_pos)
            if(NOT namespace_pos EQUAL -1)
                string(SUBSTRING "${implib}" 0 ${namespace_pos} target_namespace)
            endif()
            if(${target_namespace} STREQUAL "PkgConfig")
                get_target_property(lib ${implib} INTERFACE_LINK_LIBRARIES)
            else()
                get_target_property(lib ${implib} IMPORTED_IMPLIB_RELEASE)
            endif()
            set(implib "${lib}")
        endif()
        execute_process(
            COMMAND dlltool -I "${implib}"
            OUTPUT_VARIABLE dllname
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(CMAKE_C_COMPILER_ID MATCHES "Clang")
            list(APPEND ${libs_out} "${implib}")
            list(APPEND ${opts_out} "LINKER:/delayload:${dllname}")
        else()
            get_filename_component(libname "${implib}" NAME_WE)
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
            list(APPEND ${libs_out} "${PROJECT_BINARY_DIR}/${libname}-delay.a")
        endif()
    endforeach()
    return(PROPAGATE ${libs_out} ${opts_out})
endfunction()

function(delay_targets targets)
    foreach(tgt IN LISTS targets)
        if (TARGET ${tgt})
            get_target_property(real "${tgt}" ALIASED_TARGET)
            if(real)
                set(tgt "${real}")
            endif()
            string(FIND "${tgt}" "::" namespace_pos)
            if(NOT namespace_pos EQUAL -1)
                string(SUBSTRING "${tgt}" 0 ${namespace_pos} target_namespace)
            endif()
            if(${target_namespace} STREQUAL "PkgConfig")
                get_target_property(implib ${tgt} INTERFACE_LINK_LIBRARIES)
            else()
                get_target_property(implib ${tgt} IMPORTED_IMPLIB_RELEASE)
            endif()
            execute_process(
                COMMAND dlltool -I "${implib}"
                OUTPUT_VARIABLE dllname
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            if(CMAKE_C_COMPILER_ID MATCHES "Clang")
                target_link_options(${tgt} INTERFACE "LINKER:/delayload:${dllname}")
            else()
                get_filename_component(libname "${implib}" NAME_WE)
                set(dllpath "$ENV{MINGW_PREFIX}/bin/${libname}.dll")
                execute_process(
                    COMMAND gendef - "${dllpath}"
                    OUTPUT_FILE "${PROJECT_BINARY_DIR}/${libname}.def"
                    ERROR_QUIET
                )
                execute_process(
                    COMMAND dlltool -k -d "${libname}.def" -y "${libname}-delay.a"
                    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                )
                if(${target_namespace} STREQUAL "PkgConfig")
                    set_target_properties(${tgt} PROPERTIES INTERFACE_LINK_LIBRARIES "${PROJECT_BINARY_DIR}/${libname}-delay.a")
                else()
                    set_target_properties(${tgt} PROPERTIES IMPORTED_IMPLIB_RELEASE "${PROJECT_BINARY_DIR}/${libname}-delay.a")
                endif()
            endif()
        endif()
    endforeach()
endfunction()
