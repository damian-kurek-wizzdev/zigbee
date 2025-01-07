cmake_minimum_required(VERSION 3.10)


set (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)


set (ALL_SOURCES
    ${SRC_DIR}/defines.h
    ${SRC_DIR}/sleep.h
    ${SRC_DIR}/utils.c
    ${SRC_DIR}/utils.h
    ${SRC_DIR}/error_codes.c
    ${SRC_DIR}/error_codes.h
    )


set (ALL_INCLUDE_DIRS
    ${SRC_DIR}/
    )


set(COMPONENT_REQUIRES 
    log
    freertos
    )

set(COMPONENT_PRIV_REQUIRES)

set(COMPONENT_SRCS ${ALL_SOURCES})
set(COMPONENT_ADD_INCLUDEDIRS ${ALL_INCLUDE_DIRS})


register_component()

#if (${BUILD_WITH_PRINTS_AND_LOGS})
#    message(WARNING "!!! Building program with prints and logs!")
#    add_definitions(-DENABLE_PRINTS_AND_LOGS)
#endif()


#target_compile_options(${COMPONENT_TARGET} PRIVATE -std=gnu99)  // already set by idf.py
target_compile_options(${COMPONENT_TARGET} PRIVATE -Wall)
