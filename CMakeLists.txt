project(i2c_proxy CXX ASM)

add_library(i2c_proxy STATIC
    I2C.cpp
)

# Configuração do FreeRTOS
if(FREERTOS_ENABLED)
    # Adiciona I2C_freeRTOS.cpp
    target_sources(i2c_proxy PRIVATE
        freertos/I2C_freeRTOS.cpp
    )

    target_include_directories(i2c_proxy PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/freertos
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

# Variação de plataforma: se estiver usando Pico/pico-sdk, linka as libs específicas.
# Em outras plataformas, apenas log_vt100 é linkada aqui e as libs de I2C devem ser
# providas pelo executável/aplicação que usar o i2c_proxy.
if(TARGET pico_stdlib AND TARGET hardware_i2c)
    target_link_libraries(i2c_proxy PUBLIC
        pico_stdlib
        hardware_i2c
        log_vt100
    )
else()
    target_link_libraries(i2c_proxy PUBLIC
        log_vt100
    )
endif()

