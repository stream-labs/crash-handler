CMAKE_MINIMUM_REQUIRED(VERSION 3.17)
PROJECT(crash-handler-process VERSION 0.0.1)

set(CXX_STANDARD_REQUIRED ON)
IF(WIN32)
	set(CMAKE_CXX_STANDARD 20)
ELSE(APPLE)
	set(CMAKE_CXX_STANDARD 17)
ENDIF()

IF(WIN32)
	include(FetchContent)
	# Nlohmann JSON (modern JSON for C++)
	FetchContent_Declare(
	nlohmannjson
	GIT_REPOSITORY https://github.com/nlohmann/json
	)

	FetchContent_GetProperties(nlohmannjson)
	if(NOT nlohmannjson_POPULATED)
	FetchContent_Populate(nlohmannjson)
	endif()
 
	include(ExternalProject)

	ExternalProject_Add(libawscpp
		GIT_REPOSITORY    https://github.com/aws/aws-sdk-cpp.git
		GIT_TAG           1.8.186
		LIST_SEPARATOR    "|"
		CMAKE_ARGS       -DBUILD_SHARED_LIBS=OFF
				 -DBUILD_ONLY=s3|sts
				 -DENABLE_TESTING=OFF
				 -DCUSTOM_MEMORY_MANAGEMENT=OFF
				 -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_CURRENT_BINARY_DIR}/awsi
		BUILD_ALWAYS      TRUE
		TEST_COMMAND      ""
		PREFIX 		  awss
	)
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
		"${PROJECT_SOURCE_DIR}/platforms/upload-window-win.cpp" "${PROJECT_SOURCE_DIR}/platforms/upload-window-win.hpp"
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

IF(WIN32)
	# Include/link crash manager dependencies
	target_include_directories(crash-handler-process PUBLIC
		"${CMAKE_CURRENT_BINARY_DIR}/awsi/include/"
		"${nlohmannjson_SOURCE_DIR}/single_include" )
	add_compile_definitions(AWS_CRASH_UPLOAD_BUCKET_KEY=\"$ENV{AWS_CRASH_UPLOAD_BUCKET_KEY}\")

	set( AWS_LIBS
		"${CMAKE_CURRENT_BINARY_DIR}/awsi/lib/aws-c-common.lib"
		"${CMAKE_CURRENT_BINARY_DIR}/awsi/lib/aws-c-event-stream.lib"
		"${CMAKE_CURRENT_BINARY_DIR}/awsi/lib/aws-checksums.lib"
		"${CMAKE_CURRENT_BINARY_DIR}/awsi/lib/aws-cpp-sdk-core.lib"
		"${CMAKE_CURRENT_BINARY_DIR}/awsi/lib/aws-cpp-sdk-sts.lib"
		"${CMAKE_CURRENT_BINARY_DIR}/awsi/lib/aws-cpp-sdk-s3.lib"
		)
	target_link_libraries(crash-handler-process "${AWS_LIBS}")
ELSE()
	target_link_libraries(crash-handler-process ${COCOA})
ENDIF()

#############################
# Distribute
#############################
INSTALL(TARGETS crash-handler-process RUNTIME DESTINATION "./" COMPONENT Runtime )
IF(WIN32)
	INSTALL(FILES $<TARGET_PDB_FILE:crash-handler-process> DESTINATION "./" OPTIONAL)
ENDIF()
INSTALL(DIRECTORY ${PROJECT_DATA} DESTINATION "./" OPTIONAL)
