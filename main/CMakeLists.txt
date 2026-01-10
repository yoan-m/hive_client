idf_component_register(SRCS "espnow.c" "memory.c" "scale.c" "main.c" "sensors.c"
                       PRIV_REQUIRES spi_flash
                       INCLUDE_DIRS "."
    REQUIRES onewire_bus ds18b20 esp_adc)
