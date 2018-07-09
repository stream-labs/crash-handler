CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(crash-handler VERSION 0.0.1)

#############################
# Source, Libraries & Directories
#############################
SET(PROJECT_SOURCE
	"${PROJECT_SOURCE_DIR}/source/message.cpp" "${PROJECT_SOURCE_DIR}/source/message.hpp"
	"${PROJECT_SOURCE_DIR}/source/namedsocket-win.cpp" "${PROJECT_SOURCE_DIR}/source/namedsocket-win.hpp"
	"${PROJECT_SOURCE_DIR}/source/namedsocket.cpp" "${PROJECT_SOURCE_DIR}/source/namedsocket.hpp"
	"${PROJECT_SOURCE_DIR}/source/main.cpp"
)

#############################
# Building
#############################
ADD_EXECUTABLE(crash-handler ${PROJECT_SOURCE})
