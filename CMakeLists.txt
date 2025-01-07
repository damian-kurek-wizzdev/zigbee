cmake_minimum_required(VERSION 3.10)

set(PROJECT_VER "0.1.0")

set(SUPPORTED_TARGETS esp32)


set(EXTRA_COMPONENT_DIRS main/ main/components ${ESP_ADDITIONAL_COMPONENTS})

set(IDF_CMAKE ${CMAKE_SOURCE_DIR}/externals/esp-idf/tools/cmake/project.cmake)
# set(IDF_CMAKE $ENV{IDF_PATH}/tools/cmake/project.cmake)  # TODO - check if should replace the command above

include(${IDF_CMAKE})

# option(BUILD_WITH_PRINTS_AND_LOGS "Enable printing log messages" OFF)

project(zigbee)
