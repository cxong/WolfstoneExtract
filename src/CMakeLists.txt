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
	resourcefile.cpp
	scanner.cpp
	soundbank.cpp
	zstring.cpp
)

set_target_properties(wolfstoneextract PROPERTIES CXX_STANDARD 17)

target_link_libraries(wolfstoneextract ooz ww2ogg)
