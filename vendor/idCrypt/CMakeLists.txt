add_library(idCrypt STATIC idCrypt.c)

target_include_directories(idCrypt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(WIN32)
	target_sources(idCrypt PRIVATE idCrypt_win32.c)
else()
	target_sources(idCrypt PRIVATE idCrypt_linux.c)
endif()
