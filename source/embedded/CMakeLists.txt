##
# @file CMakeLists.txt
# @brief signal_light_demo embedded sources
#/

set(APP_PATH ${CMAKE_CURRENT_LIST_DIR})
get_filename_component(APP_NAME ${APP_PATH} NAME)

if (CONFIG_LVGL_PC_SIMULATOR STREQUAL "y")
    add_library(${EXAMPLE_LIB})
    target_compile_definitions(${EXAMPLE_LIB} PRIVATE LVGL_PC_SIMULATOR)
    target_include_directories(${EXAMPLE_LIB} PRIVATE ${APP_PATH}/include)
else()

set(APP_SRCS
    ${APP_PATH}/src/tuya_main.c
    ${APP_PATH}/src/app_chat_bot.c
    ${APP_PATH}/src/agent_status.c
    ${APP_PATH}/src/signal_led.c
    ${APP_PATH}/src/signal_oled.c
    ${APP_PATH}/src/signal_transport.c
    ${APP_PATH}/src/signal_ws.c
    ${APP_PATH}/src/signal_discovery.c
    ${APP_PATH}/src/signal_voice_key.c
    ${APP_PATH}/src/signal_mock.c
    ${APP_PATH}/src/reset_netcfg.c
)

set(APP_INC ${APP_PATH}/include)

add_library(${EXAMPLE_LIB})

target_sources(${EXAMPLE_LIB}
    PRIVATE
        ${APP_SRCS}
)

target_include_directories(${EXAMPLE_LIB}
    PRIVATE
        ${APP_INC}
)

endif()
