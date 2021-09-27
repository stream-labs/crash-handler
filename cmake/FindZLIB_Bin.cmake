if (ZLIB_LIB)
	GET_FILENAME_COMPONENT(ZLIB_BIN_PATH ${ZLIB_LIB} PATH)
endif()

file(GLOB ZLIB_BIN_FILES
	"${ZLIB_BIN_PATH}/zlib*.dll")

if (NOT ZLIB_BIN_FILES)
	file(GLOB ZLIB_BIN_FILES
		"${ZLIB_INCLUDE_DIR}/../bin${_bin_suffix}/zlib*.dll"
		"${ZLIB_INCLUDE_DIR}/../bin/zlib*.dll"
		"${ZLIB_INCLUDE_DIR}/bin${_bin_suffix}/zlib*.dll"
		"${ZLIB_INCLUDE_DIR}/bin/zlib*.dll"
		)
endif()

message(STATUS "zlib files: ${ZLIB_BIN_FILES}")
