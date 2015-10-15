cmake_minimum_required(VERSION 2.6)

project(muparser CXX)

include_directories(
	include
)

file(
	GLOB
	source_files
	src/*.cpp
)

add_library(
	muparser
	SHARED
	${source_files}
)

file(
	GLOB
	header_files
	include/*
)

install(
    TARGETS muparser
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
	FILES ${header_files} DESTINATION include
)