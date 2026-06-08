# PlatformIO — setup, entornos y testing

## platformio.ini de referencia
```ini
[platformio]
default_envs = esp32dev

[env]
build_flags = -std=gnu++17
build_unflags = -std=gnu++11

[env:esp32dev]
platform = espressif32
board = esp32doit-devkit-v1      ; ESP32 DevKit V1 (WROOM-32)
framework = arduino
monitor_speed = 115200
monitor_filters = esp32_exception_decoder, time
lib_deps =
    vshymanskyy/TinyGSM @ ^0.12.0          ; SIM800L (AT/SMS/GPRS) detrás de IGsm
    dfrobot/DFRobotDFPlayerMini @ ^1.0.6   ; DFPlayer detrás de IAudioPlayer
; SD y WiFi vienen incluidas en el framework arduino-esp32

[env:native]
platform = native
test_framework = unity
build_flags = -std=gnu++17
```

Notas:
- `board`: `esp32doit-devkit-v1` para el DevKit V1; `esp32dev` es el genérico equivalente.
- `monitor_filters = esp32_exception_decoder` traduce los *crash dumps* a líneas de código: imprescindible para depurar.
- `TinyGSM` maneja el SIM800L de forma robusta; igual se envuelve detrás de la interfaz `IGsm` para no acoplar el resto del código a la librería.

## Estructura de un componente en lib/
Cada módulo es una carpeta con su `.h` y `.cpp`. PlatformIO lo compila solo si alguien lo `#include`.
```text
lib/
└── Sim800Driver/
    ├── IGsm.h            # interfaz (contrato)
    ├── Sim800Driver.h    # implementación concreta
    └── Sim800Driver.cpp
```
Para usarlo desde `src/main.cpp` o desde otro componente: `#include <Sim800Driver.h>`.

## Testing en env:native (sin ESP32)
La lógica pura corre en la PC. El hardware se sustituye por *mocks* que implementan la misma interfaz.

`test/test_phone/test_phone.cpp`:
```cpp
#include <unity.h>
#include "PhoneUtils.h"

void test_valid_ec_number(void) {
    TEST_ASSERT_TRUE(validatePhoneNumber("+593999999999"));
}
void test_rejects_garbage(void) {
    TEST_ASSERT_FALSE(validatePhoneNumber("hola"));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_valid_ec_number);
    RUN_TEST(test_rejects_garbage);
    return UNITY_END();
}
```

Ejecutar:
```bash
pio test -e native        # tests en la PC
pio test -e esp32dev      # tests que sí requieren el chip
pio run -e esp32dev -t upload
pio device monitor
```

## Por qué interfaces + mocks
Si `AlarmController` dependiera de `Sim800Driver` concreto, no podrías probar la máquina de estados sin un SIM800L conectado. Dependiendo de `IGsm`, en el test le pasas un `FakeGsm` que solo registra "se llamó sendSms" y verificas la lógica en segundos, en la PC.
