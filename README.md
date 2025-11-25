# I2C_Proxy

![Visitantes do Projeto](https://visitor-badge.laobi.icu/badge?page_id=arvoredossaberes.i2c_proxy)
[![License: CC BY 4.0](https://img.shields.io/badge/License-CC%20BY%204.0-lightgrey.svg)](LICENSE)
![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Pico SDK](https://img.shields.io/badge/Raspberry%20Pi-Pico%20SDK-brightgreen)
![CMake](https://img.shields.io/badge/CMake-%3E%3D3.16-informational)
[![Docs](https://img.shields.io/badge/docs-Doxygen-blueviolet)](docs/index.html)
[![Latest Release](https://img.shields.io/github/v/release/ArvoreDosSaberes/I2C_Proxy?label=version)](https://github.com/ArvoreDosSaberes/I2C_Proxy/releases/latest)

Biblioteca que fornece um **wrapper C++ simples para o periférico I2C** do RP2040/RP2350, com uma API inspirada em `Wire`/Arduino, além de **funções auxiliares para uso com FreeRTOS** (semáforos por porta I2C).

Pensada para ser usada em conjunto com outros módulos do ecossistema (por exemplo `oled_ssd1306`, sensores I2C diversos e exemplos com FreeRTOS).

## Visão geral

- **`I2C.hpp` / `I2C.cpp`**: classe C++ `I2C` que encapsula as chamadas ao SDK do Pico (`hardware/i2c.h`).
- **`I2C_freeRTOS.hpp` / `I2C_freeRTOS.cpp`**: funções C para sincronizar acesso ao barramento I2C usando **semáforos FreeRTOS**, permitindo uso seguro entre múltiplas tasks.

Principais objetivos:

- Simplificar a configuração e uso do I2C (pinos, frequência, begin/end etc.).
- Oferecer uma interface de alto nível para **escrita/leitura** de dispositivos I2C.
- Facilitar o compartilhamento do mesmo barramento entre tarefas FreeRTOS, evitando condições de corrida.

## Classe `I2C`

Declaração principal (resumo):

```cpp
class I2C {
public:
    I2C(i2c_inst_t *i2c_instance, uint sda_pin, uint scl_pin);

    void begin();
    void end();
    void setClock(uint frequency);

    void    beginTransmission(uint8_t address, bool nostop = false);
    uint8_t endTransmission(void);

    size_t  write(uint8_t data);
    size_t  write(const uint8_t *data, size_t quantity);

    uint8_t requestFrom(uint8_t address, size_t quantity, bool nostop = false);
    int     read(void);
    void    read(uint8_t *buffer, uint len);
    int     available(void);
};
```

### Conceitos principais

- **Construção**:
  - Recebe o ponteiro da instância I2C (`i2c0`, `i2c1` etc.) e os pinos SDA/SCL.
- **`begin()`**:
  - Inicializa o periférico e configura os GPIOs para função I2C.
- **`end()`**:
  - Desabilita o periférico e devolve os GPIOs ao estado padrão.
- **`setClock(frequency)`**:
  - Ajusta a frequência do clock I2C (em Hz), por exemplo `100000`, `400000`.
- **Fluxo de escrita**:
  - `beginTransmission(address)` → `write(...)` → `endTransmission()`.
- **Fluxo de leitura**:
  - `requestFrom(address, quantity)` → ler com `read()` ou `read(buffer, len)`.

Os buffers internos de TX/RX são limitados (32 bytes), adequados para exemplos e protocolos simples.

## FreeRTOS: controle de acesso ao barramento

Para uso em ambientes com **FreeRTOS**, a biblioteca fornece funções auxiliares em C (interface estável para ser chamada tanto de C quanto de C++):

```c
BaseType_t initI2CSemaphore(uint8_t port);
BaseType_t takeI2C(uint8_t port, TickType_t delay);
BaseType_t releaseI2C(uint8_t port);
```

- **`initI2CSemaphore(port)`**: deve ser chamada na inicialização para criar o semáforo correspondente ao barramento/porta indicada.
- **`takeI2C(port, delay)`**: tenta obter o semáforo do barramento; bloqueia até `delay` *ticks*.
- **`releaseI2C(port)`**: libera o semáforo, permitindo que outra task utilize o barramento.

> A ideia é que **todas as tasks** que acessam o mesmo barramento chamem `takeI2C` antes de operações I2C e `releaseI2C` depois, garantindo exclusão mútua.

### Macro de compilação `I2C_USE_FREERTOS`

No código da classe `I2C` existe o uso condicional de:

```cpp
#ifdef I2C_USE_FREERTOS
    #include "I2C_freeRTOS.hpp"
    #include "FreeRTOS.h"
#endif
```

Ou seja:

- **Quando `I2C_USE_FREERTOS` está definida** (por `-DI2C_USE_FREERTOS` no CMake ou `#define I2C_USE_FREERTOS` antes dos includes):
  - A biblioteca passa a integrar-se com as rotinas de semáforo de `I2C_freeRTOS.hpp`.
  - O fluxo típico é: `takeI2C` / operações I2C / `releaseI2C`.

- **Quando `I2C_USE_FREERTOS` NÃO está definida**:
  - A classe `I2C` funciona em modo "bare metal", sem dependência direta de FreeRTOS.
  - O controle de concorrência (se necessário) fica a cargo da aplicação.

## Exemplo de uso básico (sem FreeRTOS)

```cpp
#include "pico/stdlib.h"
#include "I2C.hpp"

int main() {
    stdio_init_all();

    // Instancia para i2c0 nos pinos SDA=16, SCL=17
    I2C bus(i2c0, 16, 17);
    bus.begin();
    bus.setClock(400000); // 400 kHz

    // Escreve um registro em um dispositivo no endereco 0x3C
    bus.beginTransmission(0x3C);
    bus.write(0x00);      // Exemplo: registrador/comando
    bus.write(0xAF);      // Exemplo: dado
    bus.endTransmission();

    while (true) {
        tight_loop_contents();
    }
}
```

## Exemplo conceitual com FreeRTOS

```c
// Arquivo C, usando as funcoes auxiliares para sincronizar o barramento
#include "FreeRTOS.h"
#include "task.h"
#include "I2C_freeRTOS.hpp"

void i2c_task(void *pvParameters) {
    const uint8_t port = 0; // por exemplo, associar 0 a i2c0

    initI2CSemaphore(port);

    for (;;) {
        if (takeI2C(port, pdMS_TO_TICKS(100))) {
            // Aqui faz operacoes I2C de forma exclusiva
            // ...
            releaseI2C(port);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

No lado C++, a task pode combinar `takeI2C`/`releaseI2C` com o uso da classe `I2C` para acesso organizado.

## Integração com CMake

Um exemplo típico de integracao (ajuste conforme o `CMakeLists.txt` deste repositório):

```cmake
add_library(I2C_Proxy STATIC
    I2C.cpp
    I2C_freeRTOS.cpp
)

target_include_directories(I2C_Proxy PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(I2C_Proxy
    pico_stdlib
    hardware_i2c
    FreeRTOS-Kernel-Heap4    # se usar FreeRTOS
)
```

No projeto que consome esta biblioteca:

```cmake
target_link_libraries(seu_alvo
    pico_stdlib
    I2C_Proxy
)
```

## Uso dentro do workspace Keyboard-Menu

Dentro do workspace `Keyboard-Menu---workspace`, esta biblioteca pode ser usada como **camada base de I2C** para:

- Drivers de sensores conectados ao barramento I2C.
- A biblioteca `oled_ssd1306` (para exibir menus e informacoes).
- Exemplos que combinem **FreeRTOS**, teclado (`Keyboard`), menu (`Keyboard_Menu_FreeRTOS`) e display.

## Licença

Consulte o arquivo `LICENSE` neste diretório para detalhes de uso e redistribuicao.
