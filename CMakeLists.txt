cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME CubeMX_Config)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # BSP layer - Board configuration
    bsp/boards/alientek_explorer_v2.2/board.c

    # BSP layer - Components
    bsp/components/lcd/nt35510/lcd.c
    bsp/components/touch/gt9147/gt9147.c
    bsp/components/uart/uart_dbg/uart_dbg.c

    # Application layer
    app/ports/lv_port_disp.c
    app/ports/lv_port_indev.c

    # LVGL Demo
    app/demos/demo_main.c
    app/demos/demo_simple.c
    app/demos/demo_basic.c
    app/demos/demo_input.c
    app/demos/demo_container.c
    app/demos/demo_advanced.c
)

# LVGL 8.3.11 源码
file(GLOB_RECURSE LVGL_SRC CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/lvgl/src/*.c
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${LVGL_SRC})

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Configuration files
    ${CMAKE_CURRENT_SOURCE_DIR}/config

    # BSP layer includes
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/boards/alientek_explorer_v2.2
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/components/lcd/nt35510
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/components/touch/gt9147
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp/components/uart/uart_dbg

    # Application layer includes
    ${CMAKE_CURRENT_SOURCE_DIR}/app/core
    ${CMAKE_CURRENT_SOURCE_DIR}/app/tasks
    ${CMAKE_CURRENT_SOURCE_DIR}/app/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/app/ports
    ${CMAKE_CURRENT_SOURCE_DIR}/app/demos
    ${CMAKE_CURRENT_SOURCE_DIR}/app/inc

    # LVGL middleware
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/Third_Party/lvgl

    # Root (for backward compatibility during transition)
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    LV_CONF_INCLUDE_SIMPLE
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
