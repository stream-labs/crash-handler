cmake_minimum_required(VERSION 3.0)
project(crash_handler)

include(NodeJS)
include(CPack)

nodejs_init()

add_nodejs_module(${PROJECT_NAME} module.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_PATHS})
target_compile_definitions(${PROJECT_NAME} PRIVATE BUILDING_NODE_EXTENSION)

#############################
# Distribute
#############################
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "./" COMPONENT Runtime
	LIBRARY DESTINATION "./" COMPONENT Runtime
)

install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION ./ OPTIONAL)
install(FILES "${PROJECT_SOURCE_DIR}/package.json" DESTINATION "./")
install(FILES "${PROJECT_SOURCE_DIR}/main.js" DESTINATION "./")
