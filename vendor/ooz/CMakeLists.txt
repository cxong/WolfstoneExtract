add_library(ooz STATIC
	bitknit.cpp
	kraken.cpp
	lzna.cpp
)

target_include_directories(ooz PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
