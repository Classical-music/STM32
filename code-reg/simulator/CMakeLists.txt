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
    "${UI_ROOT}/components/ui_page_base.c"
    "${UI_ROOT}/components/ui_param.c"
    "${UI_ROOT}/components/ui_popup.c"
    "${UI_ROOT}/pages/page_home.c"
    "${UI_ROOT}/pages/page_beep.c"
    "${UI_ROOT}/pages/page_calendar.c"
    "${UI_ROOT}/pages/page_led.c"
    "${UI_ROOT}/pages/page_lvgl_perf.c"
    "${UI_ROOT}/views/views.c"
)

# 与目标板同源的纯 C 任务层代码 + 其对 bsp 层的模拟器桩实现
set(TASK_SOURCES
    "${PROJECT_ROOT}/src/tasks/rtc/task_rtc.c"
    "${CMAKE_CURRENT_LIST_DIR}/stubs/rtc/bsp_rtc_sim.c"
)

add_executable(code_reg_lvgl_simulator
    main.c
    "${WINDOWS_PORT}/lv_port_disp.c"
    "${WINDOWS_PORT}/lv_port_indev.c"
    ${LVGL_SOURCES}
    ${UI_SOURCES}
    ${TASK_SOURCES}
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
    "${PROJECT_ROOT}/src/tasks"
    "${PROJECT_ROOT}/src/bsp"
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
