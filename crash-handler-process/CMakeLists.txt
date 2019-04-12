cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
PROJECT(crash-handler-process VERSION 0.0.1)

include(ExternalProject)
include(DownloadProject)
include(FetchContent)

# Nlohmann JSON (modern JSON for C++)
FetchContent_Declare(
  nlohmannjson
  GIT_REPOSITORY https://github.com/nlohmann/json
)

add_compile_definitions(CURL_STATICLIB)

# Curl for people
FetchContent_Declare(
  cpr
  GIT_REPOSITORY https://github.com/whoshuu/cpr/
)

FetchContent_GetProperties(nlohmannjson)
if(NOT nlohmannjson_POPULATED)
  FetchContent_Populate(nlohmannjson)
endif()

set(BUILD_CURL_EXE false CACHE BOOL "" FORCE)
set(CURL_STATIC_CRT true CACHE BOOL "" FORCE)
set(CURL_STATICLIB true CACHE BOOL "" FORCE)
set(BUILD_CPR_TESTS false CACHE BOOL "" FORCE)
set(BUILD_TESTING false CACHE BOOL "" FORCE)
set(CMAKE_USE_OPENSSL false CACHE BOOL "" FORCE)
set(CMAKE_USE_WINSSL true CACHE BOOL "" FORCE)

FetchContent_GetProperties(cpr)
if(NOT cpr_POPULATED)
  FetchContent_Populate(cpr)
  add_subdirectory(${cpr_SOURCE_DIR} ${cpr_BINARY_DIR})
endif()

############################
# Setup crashpad binaries
############################
download_project(
	PROJ crashpad
	URL https://obsstudionodes3.streamlabs.com/crashpad/crashpad-release-1.0.21-win-x64.tar.gz
	UPDATE_DISCONNECTED 1
)

ExternalProject_Add(
	crashpad_ep
	DOWNLOAD_COMMAND ""
	SOURCE_DIR "${crashpad_SOURCE_DIR}"
	INSTALL_COMMAND ""
	BUILD_COMMAND ""
	CONFIGURE_COMMAND ""
	BUILD_BYPRODUCTS
		"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}base${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}util${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"<SOURCE_DIR>/lib/${CMAKE_STATIC_LIBRARY_PREFIX}client${CMAKE_STATIC_LIBRARY_SUFFIX}"
		"<SOURCE_DIR>/bin/crashpad_database_util${CMAKE_EXECUTABLE_SUFFIX}"
		"<SOURCE_DIR>/bin/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}"
		"<SOURCE_DIR>/bin/crashpad_http_upload${CMAKE_EXECUTABLE_SUFFIX}"
)

# Our crashpad artifacts assume a particular format
# <dir>\bin contains external processes
# <dir>\lib contains static libraries we need to link against
# <dir>\include contains the primary include path
# <dir>\include\third_party\mini_chromium contains chromium include files

add_library(crashpad_util STATIC IMPORTED)
add_library(crashpad_base STATIC IMPORTED)
add_library(crashpad_client STATIC IMPORTED)
add_executable(crashpad_handler IMPORTED)
add_executable(crashpad_database_util IMPORTED)
add_executable(crashpad_http_upload IMPORTED)

# From this, we get three total targets:
#   crashpad_base
#   crashpad_util
#   crashpad_client
# It's recommended to use util but not required as far as I know.

ExternalProject_Get_Property(crashpad_ep source_dir)

set_property(TARGET crashpad_base PROPERTY IMPORTED_LOCATION
	"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}base${CMAKE_STATIC_LIBRARY_SUFFIX}")

set_property(TARGET crashpad_util PROPERTY IMPORTED_LOCATION
	"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}util${CMAKE_STATIC_LIBRARY_SUFFIX}")

set_property(TARGET crashpad_client PROPERTY IMPORTED_LOCATION
	"${source_dir}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}client${CMAKE_STATIC_LIBRARY_SUFFIX}")

set_property(TARGET crashpad_handler PROPERTY IMPORTED_LOCATION
	"${source_dir}/bin/crashpad_handler${CMAKE_EXECUTABLE_SUFFIX}")

set_property(TARGET crashpad_database_util PROPERTY IMPORTED_LOCATION
	"${source_dir}/bin/crashpad_database_util${CMAKE_EXECUTABLE_SUFFIX}")

set_property(TARGET crashpad_http_upload PROPERTY IMPORTED_LOCATION
	"${source_dir}/bin/crashpad_http_upload${CMAKE_EXECUTABLE_SUFFIX}")

target_include_directories(
	crashpad_base
	INTERFACE "${source_dir}/include/third_party/mini_chromium/mini_chromium"
)

target_include_directories(
	crashpad_client
	INTERFACE "${source_dir}/include"
)

add_library(crashpad INTERFACE)

target_link_libraries(
	crashpad
	INTERFACE
		crashpad_base
		crashpad_client
		crashpad_util
)

target_link_libraries(crashpad_util INTERFACE crashpad_client)

add_dependencies(crashpad_base crashpad_ep)
add_dependencies(crashpad_client crashpad_base)
add_dependencies(crashpad_util crashpad_client)

#############################
# Source, Libraries & Directories
#############################
SET(PROJECT_SOURCE
	"${PROJECT_SOURCE_DIR}/process.cpp" "${PROJECT_SOURCE_DIR}/process.hpp"
	"${PROJECT_SOURCE_DIR}/metricsprovider.cpp" "${PROJECT_SOURCE_DIR}/metricsprovider.hpp"
	"${PROJECT_SOURCE_DIR}/crashpadprovider.cpp" "${PROJECT_SOURCE_DIR}/crashpadprovider.hpp"
	"${PROJECT_SOURCE_DIR}/message.cpp" "${PROJECT_SOURCE_DIR}/message.hpp"
	"${PROJECT_SOURCE_DIR}/namedsocket-win.cpp" "${PROJECT_SOURCE_DIR}/namedsocket-win.hpp"
	"${PROJECT_SOURCE_DIR}/namedsocket.cpp" "${PROJECT_SOURCE_DIR}/namedsocket.hpp"
	"${PROJECT_SOURCE_DIR}/main.cpp"
)


#############################
# Building
#############################
ADD_EXECUTABLE(${PROJECT_NAME} ${PROJECT_SOURCE})

# Include/link crash manager dependencies
target_include_directories(${PROJECT_NAME} PUBLIC 
	"${nlohmannjson_SOURCE_DIR}/single_include"
	"${cpr_SOURCE_DIR}/include"
	"${crashpad_SOURCE_DIR}/include")
target_link_libraries(${PROJECT_NAME} cpr)
target_link_libraries(${PROJECT_NAME} libcurl)

target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARIES} optimized crashpad)


#############################
# Distribute
#############################
INSTALL(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "./" COMPONENT Runtime
)
INSTALL(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "./" OPTIONAL)
INSTALL(DIRECTORY ${PROJECT_DATA} DESTINATION "./" OPTIONAL)
install(DIRECTORY ${crashpad_SOURCE_DIR}/bin/ DESTINATION "./")
