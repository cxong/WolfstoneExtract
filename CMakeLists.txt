cmake_minimum_required(VERSION 3.13)
project(WolfstoneExtract
	LANGUAGES CXX C
	VERSION 1.0
)

include(GNUInstallDirs)

option(INSTALL_FLAT "Flatten install paths for loose packaging." OFF)
if(INSTALL_FLAT)
	set(CMAKE_INSTALL_BINDIR ".")
endif()

set(CPACK_PACKAGE_CONTACT "First Last <example@example.com>" CACHE STRING "Contact info for archive maintainer.")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Extraction tool for Wolfstone game data.\n Extracts Wolfstone 3D and Elite Hans from Steam installs of Wolfenstein II and\n Wolfenstein: Youngblood. Resulting files are formatted for use with ECWolf.")
string(TOLOWER "${PROJECT_NAME}" CPACK_PACKAGE_NAME)
# Use same prefix for packaging for consistency
set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
set(CPACK_STRIP_FILES ON)
set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/CPackProjectConfig.cmake")

if(WIN32)
	set(CPACK_GENERATOR "ZIP")
else()
	set(CPACK_GENERATOR "TXZ")

	if(NOT DEFINED CPACK_RPM_PACKAGE_ARCHITECTURE)
		execute_process(COMMAND uname -m OUTPUT_VARIABLE PACKAGE_ARCHITECTURE OUTPUT_STRIP_TRAILING_WHITESPACE)
	endif()
	set(CPACK_RPM_PACKAGE_ARCHITECTURE "${PACKAGE_ARCHITECTURE}" CACHE STRING "Package architecture.")
endif()

include(CPack)

add_subdirectory(vendor)
add_subdirectory(src)
