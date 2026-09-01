cmake_minimum_required(VERSION 3.20)

project(code_reg_lvgl_simulator LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(LVGL_ROOT "${PROJECT_ROOT}/src/middleware/lvgl")
set(LVGL_SRC "${LVGL_ROOT}/src")
set(UI_ROOT "${PROJECT_ROOT}/src/ui")
set(WINDOWS_PORT "${LVGL_ROOT}/port/windows")

find_package(SDL2 CONFIG REQUIRED)

file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    "${LVGL_SRC}/*.c"
)

set(UI_SOURCES
    "${UI_ROOT}/common/dock.c"
    "${UI_ROOT}/common/router.c"
    "${UI_ROOT}/common/system_bar.c"
    "${UI_ROOT}/components/ui_button.c"
    "${UI_ROOT}/pages/home.c"
    "${UI_ROOT}/pages/led_beep.c"
    "${UI_ROOT}/views/views.c"
)

add_executable(code_reg_lvgl_simulator
    main.c
    "${WINDOWS_PORT}/lv_port_disp.c"
    "${WINDOWS_PORT}/lv_port_indev.c"
    ${LVGL_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(code_reg_lvgl_simulator PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/stubs"
    "${CMAKE_CURRENT_LIST_DIR}/stubs/beep"
    "${WINDOWS_PORT}"
    "${LVGL_ROOT}"
    "${LVGL_SRC}"
    "${UI_ROOT}"
    "${PROJECT_ROOT}/src"
)

target_compile_definitions(code_reg_lvgl_simulator PRIVATE
    LV_CONF_INCLUDE_SIMPLE
)

target_link_libraries(code_reg_lvgl_simulator PRIVATE SDL2::SDL2)

if(TARGET SDL2::SDL2main)
    target_link_libraries(code_reg_lvgl_simulator PRIVATE SDL2::SDL2main)
endif()

if(MSVC)
    target_compile_options(code_reg_lvgl_simulator PRIVATE /W4)
else()
    target_compile_options(code_reg_lvgl_simulator PRIVATE -Wall -Wextra)
endif()
