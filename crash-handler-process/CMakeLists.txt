CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(crash-handler-process VERSION 0.0.1)

include(FetchContent)

IF(WIN32)
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
ENDIF()

#############################
# Source, Libraries & Directories
#############################
SET(PROJECT_SOURCE
	"${PROJECT_SOURCE_DIR}/process.hpp"
	"${PROJECT_SOURCE_DIR}/process-manager.cpp" "${PROJECT_SOURCE_DIR}/process-manager.hpp"
	"${PROJECT_SOURCE_DIR}/message.cpp" "${PROJECT_SOURCE_DIR}/message.hpp"
	"${PROJECT_SOURCE_DIR}/socket.hpp"
	"${PROJECT_SOURCE_DIR}/logger.cpp" "${PROJECT_SOURCE_DIR}/logger.hpp"
	"${PROJECT_SOURCE_DIR}/main.cpp"
	"${PROJECT_SOURCE_DIR}/util.hpp"
)

IF(WIN32)
	SET(WINDOWS_SOURCE
		"${PROJECT_SOURCE_DIR}/platforms/util-win.cpp"
		"${PROJECT_SOURCE_DIR}/platforms/socket-win.cpp" "${PROJECT_SOURCE_DIR}/platforms/socket-win.hpp"
		"${PROJECT_SOURCE_DIR}/platforms/process-win.cpp" "${PROJECT_SOURCE_DIR}/platforms/process-win.hpp"
	)
ELSE(APPLE)
	SET(APPLE_SOURCE
		"${PROJECT_SOURCE_DIR}/platforms/util-osx.mm"
		"${PROJECT_SOURCE_DIR}/platforms/socket-osx.cpp" "${PROJECT_SOURCE_DIR}/platforms/socket-osx.hpp"
		"${PROJECT_SOURCE_DIR}/platforms/process-osx.mm" "${PROJECT_SOURCE_DIR}/platforms/process-osx.hpp"
	)
ENDIF()

find_library(COCOA Cocoa)


#############################
# Building
#############################
IF(WIN32)
	ADD_DEFINITIONS(-DUNICODE)
ENDIF()

IF(WIN32)
	ADD_EXECUTABLE(crash-handler-process ${PROJECT_SOURCE} ${WINDOWS_SOURCE})
ELSE(APPLE)
	ADD_EXECUTABLE(crash-handler-process ${PROJECT_SOURCE} ${APPLE_SOURCE})
ENDIF()

set_property(TARGET crash-handler-process PROPERTY CXX_STANDARD 17)
set_property(TARGET crash-handler-process PROPERTY CXX_STANDARD_REQUIRED ON)

IF(WIN32)
	# Include/link crash manager dependencies
	target_include_directories(crash-handler-process PUBLIC
		"${nlohmannjson_SOURCE_DIR}/single_include"
		"${cpr_SOURCE_DIR}/include")
	target_link_libraries(crash-handler-process cpr)
	target_link_libraries(crash-handler-process libcurl)
ELSE()
	target_link_libraries(crash-handler-process ${COCOA})
ENDIF()

#############################
# Distribute
#############################
INSTALL(TARGETS crash-handler-process RUNTIME DESTINATION "./crash-handler" COMPONENT Runtime )
IF(WIN32)
	INSTALL(FILES $<TARGET_PDB_FILE:crash-handler-process> DESTINATION "./crash-handler" OPTIONAL)
ENDIF()
INSTALL(DIRECTORY ${PROJECT_DATA} DESTINATION "./crash-handler" OPTIONAL)
