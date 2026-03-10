# Collect transitive target dependencies from a starting list.
function(_collect_target_deps out_var)
    set(queue ${ARGN})
    set(seen "")
    set(result "")

    while(queue)
        list(POP_FRONT queue cur)

        # Skip non-target entries (plain libs like crypt32, flags, etc.)
        if(NOT TARGET "${cur}")
            continue()
        endif()

        # Resolve aliases
        get_target_property(real "${cur}" ALIASED_TARGET)
        if(real)
            set(cur "${real}")
        endif()

        if(cur IN_LIST seen)
            continue()
        endif()
        list(APPEND seen "${cur}")
        list(APPEND result "${cur}")

        # Modern and legacy properties where imported deps are usually stored
        set(_deps "")
        get_target_property(tmp "${cur}" INTERFACE_LINK_LIBRARIES)
        if(tmp)
            list(APPEND _deps ${tmp})
        endif()
        get_target_property(tmp "${cur}" IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE)
        if(tmp)
            list(APPEND _deps ${tmp})
        endif()

        foreach(dep IN LISTS _deps)
            # Unwrap common generator expressions that hide target names
            string(REGEX REPLACE "^\\$<LINK_ONLY:(.*)>$" "\\1" dep "${dep}")
            string(REGEX REPLACE "^\\$<TARGET_NAME_IF_EXISTS:(.*)>$" "\\1" dep "${dep}")

            if(TARGET "${dep}" AND NOT dep IN_LIST seen)
                list(APPEND queue "${dep}")
            endif()
        endforeach()
    endwhile()

    set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

# Delay load exported dependency targets
function(delay_load targets)
    _collect_target_deps(all_targets ${targets})

    foreach(target IN LISTS all_targets)
        if(TARGET ${target})
            get_target_property(dllpath ${target} IMPORTED_LOCATION_RELEASE)
            get_target_property(implib ${target} IMPORTED_IMPLIB_RELEASE)
            if(NOT implib OR NOT dllpath)
                continue()
            endif()
            if(CMAKE_C_COMPILER_ID MATCHES "Clang")
                cmake_path(GET dllpath FILENAME dllname)
                target_link_options(${target} INTERFACE "LINKER:/delayload:${dllname}")
            else()
                get_filename_component(libname "${implib}" NAME_WE)
                execute_process(
                    COMMAND gendef - "${dllpath}"
                    OUTPUT_FILE "${PROJECT_BINARY_DIR}/${libname}.def"
                    RESULT_VARIABLE result
                    ERROR_QUIET
                )
                if(NOT ${result} EQUAL 0)
                    continue()
                endif()
                execute_process(
                    COMMAND dlltool -k -d "${libname}.def" -y "${libname}-delay.a"
                    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
                )
                set_target_properties(${target} PROPERTIES
                    IMPORTED_IMPLIB_RELEASE "${PROJECT_BINARY_DIR}/${libname}-delay.a"
                )
            endif()
        endif()
    endforeach()
endfunction()
