CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(crash-handler-process VERSION 0.0.1)

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

#############################
# Source, Libraries & Directories
#############################
SET(PROJECT_SOURCE
	"${PROJECT_SOURCE_DIR}/process.cpp" "${PROJECT_SOURCE_DIR}/process.hpp"
	"${PROJECT_SOURCE_DIR}/metricsprovider.cpp" "${PROJECT_SOURCE_DIR}/metricsprovider.hpp"
	"${PROJECT_SOURCE_DIR}/message.cpp" "${PROJECT_SOURCE_DIR}/message.hpp"
	"${PROJECT_SOURCE_DIR}/namedsocket-win.cpp" "${PROJECT_SOURCE_DIR}/namedsocket-win.hpp"
	"${PROJECT_SOURCE_DIR}/namedsocket.cpp" "${PROJECT_SOURCE_DIR}/namedsocket.hpp"
	"${PROJECT_SOURCE_DIR}/main.cpp"
)


#############################
# Building
#############################
ADD_DEFINITIONS(-DUNICODE)
ADD_EXECUTABLE(crash-handler-process ${PROJECT_SOURCE})

# Include/link crash manager dependencies
target_include_directories(crash-handler-process PUBLIC 
	"${nlohmannjson_SOURCE_DIR}/single_include"
	"${cpr_SOURCE_DIR}/include")
target_link_libraries(crash-handler-process cpr)
target_link_libraries(crash-handler-process libcurl)

#############################
# Distribute
#############################
INSTALL(TARGETS crash-handler-process RUNTIME DESTINATION "./crash-handler-module" COMPONENT Runtime )
#INSTALL(FILES $<TARGET_PDB_FILE:crash-handler-process> DESTINATION "./crash-handler-module" OPTIONAL)
INSTALL(DIRECTORY ${PROJECT_DATA} DESTINATION "./crash-handler-module" OPTIONAL)
