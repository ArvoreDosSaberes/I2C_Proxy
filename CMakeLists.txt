add_library(i2c_proxy STATIC
    I2C.cpp
)

# Configuração do FreeRTOS
if(FREERTOS_ENABLED)
    # Adiciona I2C_freeRTOS.cpp
    target_sources(i2c_proxy PRIVATE
        I2C_freeRTOS.cpp
    )
    
    # Define a macro de uso do FreeRTOS
    target_compile_definitions(i2c_proxy PUBLIC
        I2C_USE_FREERTOS=1
    )
    
    # Configura includes e links do FreeRTOS (centralizado)
    link_freertos(i2c_proxy)
else()
    # FreeRTOS não está disponível
    target_compile_definitions(i2c_proxy PUBLIC
        I2C_USE_FREERTOS=0
    )
endif()

target_include_directories(i2c_proxy PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_SOURCE_DIR}
)

target_link_libraries(i2c_proxy PUBLIC
    pico_stdlib
    hardware_i2c
    log
)

