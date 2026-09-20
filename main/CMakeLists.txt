idf_component_register(
    SRCS "main.c"
         "worry_seed_app.c"
         "worry_seed_model.c"
         "worry_seed_store.c"
         "worry_seed_audio.c"
         "worry_seed_adpcm.c"
         "worry_font_16.c"
         "worry_font_24.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer nvs_flash spiffs
)
