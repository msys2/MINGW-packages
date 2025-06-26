cmake_minimum_required(VERSION 3.20)

project(lsqlite3 C)

if (NOT DEFINED LSQLITE3_VERSION)
	message(FATAL_ERROR "LSQLITE3_VERSION must be set")
endif()

find_package(SQLite3 REQUIRED)

enable_testing()

include(MSYS2-GetLuaInfoFromPkgConfig-v1.0.0.cmake)

set(LSQLITE3_SOURCES "${CMAKE_SOURCE_DIR}/lsqlite3.c")
set(LSQLITE3_SOURCE_TEST_FILE "${CMAKE_SOURCE_DIR}/test/test.lua")
set(LSQLITE3_SQLITE3_SOURCE_TEST_FILE "${CMAKE_SOURCE_DIR}/test/tests-sqlite3.lua")
set(LSQLITE3_SQLITE3_TEST_ARG "lsqlite3")

set(LUNIT_ROOT_DIR "lunit" CACHE PATH "lunit root directory")
set(LUNIT_DIR "${LUNIT_ROOT_DIR}/lua")
set(LUNIT_LUA "${LUNIT_DIR}/lunit.lua")
set(LUNIT_CONSOLE_LUA "${LUNIT_DIR}/lunit/console.lua")

foreach(_file ${LSQLITE3_SOURCES} ${LSQLITE3_SOURCE_TEST_FILE} ${LSQLITE3_SQLITE3_SOURCE_TEST_FILE} ${LUNIT_LUA} ${LUNIT_CONSOLE_LUA})
	if (NOT EXISTS "${_file}")
		message(FATAL_ERROR "File not found: ${_file}")
	endif()
endforeach()

get_filename_component(LSQLITE3_TEST_FILE ${LSQLITE3_SOURCE_TEST_FILE} ABSOLUTE)
get_filename_component(LSQLITE3_SQLITE3_TEST_FILE ${LSQLITE3_SQLITE3_SOURCE_TEST_FILE} ABSOLUTE)

add_library(lsqlite3 SHARED ${LSQLITE3_SOURCES})
target_include_directories(lsqlite3 PRIVATE ${Lua_includedir} ${SQLite3_INCLUDE_DIRS})
target_compile_definitions(lsqlite3 PRIVATE "LSQLITE_VERSION=\"${LSQLITE3_VERSION}\"")
target_link_libraries(lsqlite3 PRIVATE ${Lua_DLL} ${SQLite3_LIBRARIES})

set_target_properties(lsqlite3
	PROPERTIES
	PREFIX ""
)

configure_file(${LUNIT_LUA} "${CMAKE_BINARY_DIR}/lunit.lua" COPYONLY)
configure_file(${LUNIT_CONSOLE_LUA} "${CMAKE_BINARY_DIR}/lunit/console.lua" COPYONLY)

add_test(NAME lsqlite3_test
	COMMAND ${Lua_EXE} ${LSQLITE3_TEST_FILE})

add_test(NAME lsqlite3_sqlite3_test
	COMMAND ${Lua_EXE} ${LSQLITE3_SQLITE3_TEST_FILE} ${LSQLITE3_SQLITE3_TEST_ARG})

install(FILES $<TARGET_FILE:lsqlite3>
	DESTINATION ${RELATIVE_Lua_INSTALL_CMOD})
