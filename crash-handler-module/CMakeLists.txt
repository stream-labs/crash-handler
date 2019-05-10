cmake_minimum_required(VERSION 3.0)
project(crash-handler)

include(NodeJS)
include(CPack)

nodejs_init()

add_nodejs_module(${PROJECT_NAME} module.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_PATHS})
target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDING_NODE_EXTENSION)

# Unicode support
ADD_DEFINITIONS(-DUNICODE)

#############################
# Distribute
#############################
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "./crash-handler" COMPONENT Runtime
	LIBRARY DESTINATION "./crash-handler" COMPONENT Runtime
)

install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "./crash-handler" OPTIONAL)
install(FILES "${PROJECT_SOURCE_DIR}/package.json" DESTINATION "./crash-handler")
install(FILES "${PROJECT_SOURCE_DIR}/main.js" DESTINATION "./crash-handler")
