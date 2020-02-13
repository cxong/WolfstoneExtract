set(CodebookName packed_codebooks_aoTuV_603)

file(READ "${CodebookName}.bin" Codebook HEX)
string(LENGTH "${Codebook}" CodebookSize)
string(REGEX REPLACE "([0-9A-Za-z][0-9A-Za-z])" "0x\\1," Codebook "${Codebook}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${CodebookName}.c"
	"const unsigned char ${CodebookName}[${CodebookSize}u/2] = {${Codebook}};"
)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${CodebookName}.h"
	"extern \"C\" const unsigned char ${CodebookName}[${CodebookSize}u/2];"
)

add_library(ww2ogg STATIC
	src/codebook.cpp
	src/crc.c
	src/wwriff.cpp
	"${CMAKE_CURRENT_BINARY_DIR}/${CodebookName}.c"
)

target_include_directories(ww2ogg PUBLIC src "${CMAKE_CURRENT_BINARY_DIR}")
