include(CheckFunctionExists)

check_function_exists(stricmp STRICMP_EXISTS)
check_function_exists(strnicmp STRNICMP_EXISTS)
if(NOT STRICMP_EXISTS)
	add_definitions(-Dstricmp=strcasecmp)
endif()
if(NOT STRNICMP_EXISTS)
	add_definitions(-Dstrnicmp=strncasecmp)
endif()

add_executable(wolfstoneextract
	file_idcl.cpp
	files.cpp
	filesys_steam.cpp
	filesys.cpp
	main.cpp
	mz_crc32.cpp
	resourcefile.cpp
	scanner.cpp
	soundbank.cpp
	zip.cpp
	zstring.cpp
)

set_target_properties(wolfstoneextract PROPERTIES CXX_STANDARD 17)

target_link_libraries(wolfstoneextract idCrypt ooz revorb ww2ogg)

set_source_files_properties(main.cpp PROPERTIES COMPILE_DEFINITIONS "TOOL_VERSION=\"${PROJECT_VERSION}\"")

if(MSVC)
	target_compile_definitions(wolfstoneextract PUBLIC -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_WARNINGS)
endif()

install(TARGETS wolfstoneextract DESTINATION ${CMAKE_INSTALL_BINDIR})
