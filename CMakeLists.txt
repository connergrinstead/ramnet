cmake_minimum_required(VERSION 3.10)

project(RamNet C)

# Shared source files
set(SOURCES
    src/main.c
    src/common/server.c
    src/common/audio.c
)

# Include directories
set(INCLUDES
    include/common
)

# Executable target
add_executable(ramnet ${SOURCES})
target_include_directories(ramnet PRIVATE ${INCLUDES})

# Find PortAudio
find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

target_include_directories(ramnet PRIVATE ${PORTAUDIO_INCLUDE_DIRS})
target_link_libraries(ramnet PRIVATE ${PORTAUDIO_LIBRARIES})

# Optional: add compiler flags if needed
target_compile_options(ramnet PRIVATE ${PORTAUDIO_CFLAGS_OTHER})

# Set output directory
set_target_properties(ramnet PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)