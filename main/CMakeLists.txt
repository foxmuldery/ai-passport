idf_component_register(
    SRCS "main.c"
         "toolbox_config.c"
         "toolbox_logic.c"
         "toolbox_model.c"
         "toolbox_net.c"
         "toolbox_portal.c"
         "toolbox_ui.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash
             esp_http_client esp_http_server json lwip mbedtls lvgl
)

target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/toolbox_font_12.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/toolbox_font_14.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/toolbox_font_20.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/toolbox_font_28.c"
)
