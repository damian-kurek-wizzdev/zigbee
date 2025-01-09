cmake_minimum_required(VERSION 3.10)


set (SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(${COMPONENT_DIR}/component_sources.cmake)

set (ALL_SOURCES
    ${MODULE_TESTABLE_SOURCES}
    ${MODULE_NON_TESTABLE_SOURCES}
    )


set (ALL_INCLUDE_DIRS
    ${MODULE_TESTABLE_INCLUDE_DIRS}
    ${MODULE_NON_TESTABLE_INCLUDE_DIRS}
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


#target_compile_options(${COMPONENT_TARGET} PRIVATE -Wall)
