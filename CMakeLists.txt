CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

# Set Module Path
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

#############################
# Configuration
#############################

# Installation
SET(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/distribute" CACHE PATH "Path to install files to")

ADD_SUBDIRECTORY(crash-handler)
ADD_SUBDIRECTORY(crash-handler-process)
