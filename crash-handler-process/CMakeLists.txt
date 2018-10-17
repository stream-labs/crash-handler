CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(crash-handler-process VERSION 0.0.1)

#############################
# Source, Libraries & Directories
#############################
SET(PROJECT_SOURCE
	"${PROJECT_SOURCE_DIR}/process.cpp" "${PROJECT_SOURCE_DIR}/process.hpp"
	"${PROJECT_SOURCE_DIR}/message.cpp" "${PROJECT_SOURCE_DIR}/message.hpp"
	"${PROJECT_SOURCE_DIR}/namedsocket-win.cpp" "${PROJECT_SOURCE_DIR}/namedsocket-win.hpp"
	"${PROJECT_SOURCE_DIR}/namedsocket.cpp" "${PROJECT_SOURCE_DIR}/namedsocket.hpp"
	"${PROJECT_SOURCE_DIR}/main.cpp"
)

#############################
# Building
#############################
ADD_EXECUTABLE(crash-handler-process ${PROJECT_SOURCE})

#############################
# Distribute
#############################
INSTALL(TARGETS crash-handler-process
	RUNTIME DESTINATION "./" COMPONENT Runtime
)
INSTALL(FILES $<TARGET_PDB_FILE:crash-handler-process> DESTINATION "./" OPTIONAL)
INSTALL(DIRECTORY ${PROJECT_DATA} DESTINATION "./" OPTIONAL)
