set(
        SOURCES
        main.cpp
        services/control.cpp
        services/wash.cpp
)
set(INCLUDE_DIRS .  include)

set(REQUIRES esp_wifi nvs_flash esp_event wpa_supplicant driver esp_netif lwip mbedtls spiffs)

idf_component_register(SRCS ${SOURCES} INCLUDE_DIRS ${INCLUDE_DIRS} REQUIRES ${REQUIRES})

#spiffs_create_partition_image(storage ../data FLASH_IN_PROJECT)
