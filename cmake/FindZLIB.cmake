# Once done these will be defined:
#
#  ZLIB_FOUND
#  ZLIB_INCLUDE_DIRS
#  ZLIB_LIBRARIES
#

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_ZLIB QUIET zlib)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(ZLIB_INCLUDE_DIR
	NAMES zlib.h
	HINTS
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		ENV zlibPath${_lib_suffix}
		ENV zlibPath
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${zlibPath${_lib_suffix}}
		${zlibPath}
		${_ZLIB_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		include)

find_library(ZLIB_LIB
	NAMES zlib zdll zlibd libzlib
	HINTS
		ENV DepsPath${_lib_suffix}
		ENV DepsPath
		ENV zlibPath${_lib_suffix}
		ENV zlibPath
		${DepsPath${_lib_suffix}}
		${DepsPath}
		${zlibPath${_lib_suffix}}
		${zlibPath}
		${_ZLIB_LIBRARY_DIRS}
	PATHS
		${DepsPath} /usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin
		../lib${_lib_suffix} ../lib
		../libs${_lib_suffix} ../libs
		../bin${_lib_suffix} ../bin
	NO_DEFAULT_PATH NO_SYSTEM_ENVIRONMENT_PATH NO_CMAKE_SYSTEM_PATH NO_CMAKE_ENVIRONMENT_PATH NO_CMAKE_FIND_ROOT_PATH)

message(STATUS "ZLIB_LIB 1 : ${ZLIB_LIB}")
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB DEFAULT_MSG ZLIB_LIB ZLIB_INCLUDE_DIR)
mark_as_advanced(ZLIB_INCLUDE_DIR ZLIB_LIB)
message(STATUS "ZLIB_LIB 2 : ${ZLIB_LIB}")
if(ZLIB_FOUND)
	set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
	set(ZLIB_LIBRARIES ${ZLIB_LIB})
endif()
message(STATUS "ZLIB_LIB 3 : ${ZLIB_LIB}")