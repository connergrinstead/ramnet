cmake_minimum_required(VERSION 3.10)

project(RamNet)

# Shared source files
set(SOURCES
    src/main.c
    src/common/server.c
)

# Include directories
set(INCLUDES
    include/common
)

# Regular target
add_executable(ramnet ${SOURCES})
target_include_directories(ramnet PRIVATE ${INCLUDES})
set_target_properties(ramnet PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
