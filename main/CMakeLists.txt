cmake_minimum_required(VERSION 3.10)


set (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/)


set (ALL_SOURCES
    ${SRC_DIR}/main.cpp
    )


set (ALL_INCLUDE_DIRS
    ${SRC_DIR}/
    ${SRC_DIR}/common/
    )


set(COMPONENT_REQUIRES 
    nvs_flash
    log
    freertos
    mill-zigbee
    wizzdev-common
    )


set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS ${ALL_SOURCES})
set(COMPONENT_ADD_INCLUDEDIRS ${ALL_INCLUDE_DIRS})
set(EXTRA_COMPONENT_DIRS ${SRC_DIR}/components/)

register_component()

#if (${BUILD_WITH_PRINTS_AND_LOGS})
#    message(WARNING "!!! Building program with prints and logs!")
#    add_definitions(-DENABLE_PRINTS_AND_LOGS)
#endif()


#target_compile_options(${COMPONENT_TARGET} PRIVATE -std=gnu99)  // already set by idf.py
target_compile_options(${COMPONENT_TARGET} PRIVATE -Wall)
