idf_component_register(SRCS "bringup_log.c" "bringup_psram.c" "bringup_camera.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "../../main"
                       REQUIRES log esp_psram heap esp_timer espressif__esp32-camera
                       PRIV_REQUIRES freertos soc)
