idf_component_register(SRCS "mallow_rev2.c" "draw_functions/draw_functions.c"
                    INCLUDE_DIRS "." "./draw_functions"
                    PRIV_REQUIRES spi_flash esp_timer fatfs driver
                    REQUIRES esp_lcd)
