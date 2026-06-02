cmake_minimum_required(VERSION 3.20)

project(RaylibProject LANGUAGES CXX)

# =========================
# CONFIG OPTIONS
# =========================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================
# RAYLIB PATH
# =========================

set(RAYLIB_DIR "C:/raylib/raylib/src")

# =========================
# SOURCE FILES
# =========================

set(SOURCES
    src/main.cpp
    src/startupScene.cpp
    src/menuScene.cpp
    src/gameScene.cpp
)

# =========================
# EXECUTABLE
# =========================

add_executable(RaylibProject ${SOURCES})

# =========================
# INCLUDE DIRECTORIES
# =========================

target_include_directories(RaylibProject PRIVATE
    include
    ${RAYLIB_DIR}
    ${CMAKE_SOURCE_DIR}/externals/raygui
    ${CMAKE_SOURCE_DIR}/externals/raygui/styles
)

# =========================
# RAYLIB LINKING (Windows)
# =========================

target_link_directories(RaylibProject PRIVATE
    ${RAYLIB_DIR}
)

target_link_libraries(RaylibProject PRIVATE
    raylib
    opengl32
    gdi32
    winmm
)

# =========================
# OUTPUT DIRECTORIES
# =========================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set_target_properties(RaylibProject PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_SOURCE_DIR}/bin/debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/release
)

# =========================
# DEBUG CONFIG
# =========================

target_compile_definitions(RaylibProject PRIVATE $<$<CONFIG:Debug>:DEBUG>)
target_compile_options(RaylibProject PRIVATE
    $<$<CONFIG:Debug>:-g -O0>
    $<$<CONFIG:Release>:-O2 -DNDEBUG -mwindows>
)


if(WIN32)
    set_target_properties(RaylibProject PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# =========================
# SANITIZERS
# =========================

option(ENABLE_SANITIZERS "Enable Address/UB Sanitizers" OFF)

if(ENABLE_SANITIZERS)
    target_compile_options(RaylibProject PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(RaylibProject PRIVATE
        -fsanitize=address,undefined
    )
endif()

# =========================
# ASSETS COPY
# =========================

add_custom_command(TARGET RaylibProject POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:RaylibProject>/assets
)