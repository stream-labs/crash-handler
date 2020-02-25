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

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
        )
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

#############################
# Distribute
#############################
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION "./crash-handler" COMPONENT Runtime
	LIBRARY DESTINATION "./crash-handler" COMPONENT Runtime
)

IF(WIN32)
  install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "./crash-handler" OPTIONAL)
ENDIF()
install(FILES "${PROJECT_SOURCE_DIR}/package.json" DESTINATION "./crash-handler")
install(FILES "${PROJECT_SOURCE_DIR}/main.js" DESTINATION "./crash-handler")
