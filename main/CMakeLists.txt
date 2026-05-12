set(srcs
    main.c
    speech_commands_action.c
    )

set(requires
    hardware_driver
    esp_http_client
    nvs_flash
    esp_wifi
    esp-tls
    mbedtls
    led_strip
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS include
                       REQUIRES ${requires})

component_compile_options(-w)