# Adjusts global CPACK variables for specific generators.
# See CPACK_PROJECT_CONFIG_FILE.

if(CPACK_GENERATOR STREQUAL "ZIP")
	# We just want the files flat in the archive
	set(CPACK_PACKAGING_INSTALL_PREFIX "/")
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)

	# Conform to already existing package naming convention
	if(CPACK_SYSTEM_NAME STREQUAL "win64")
		set(PACKAGE_ARCH "x64")
	else(CPACK_SYSTEM_NAME STREQUAL "win32")
		set(PACKAGE_ARCH "x86")
	endif()
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-${PACKAGE_ARCH}")
elseif(CPACK_GENERATOR STREQUAL "TXZ")
	# Also looking for a flatter archive, but not a tarbomb
	set(CPACK_PACKAGING_INSTALL_PREFIX "/")

	string(TOLOWER "${CPACK_SYSTEM_NAME}" SYSTEM_NAME)

	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${SYSTEM_NAME}-${CPACK_RPM_PACKAGE_ARCHITECTURE}")
endif()
