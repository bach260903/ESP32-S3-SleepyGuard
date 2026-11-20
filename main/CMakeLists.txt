idf_component_register(SRCS "bringup_main.c" INCLUDE_DIRS "."
                       REQUIRES esp_psram spi_flash heap log board_io bringup_logic)
