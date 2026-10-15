idf_component_register(
    SRCS "main.c"
         "sound_wave_core.c"
         "sound_wave_ui.c"
         "sound_wave_raster.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_hw_support esp_timer
)
target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/sound-wave/sound_font_14.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/sound-wave/sound_font_22.c"
)
