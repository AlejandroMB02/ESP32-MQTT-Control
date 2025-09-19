idf_component_register(SRCS "WeatherStation.c"
                    INCLUDE_DIRS "."
                    REQUIRES dht esp_wifi esp_event nvs_flash mqtt)
