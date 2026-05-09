# Medidor-Presi-n-Acondicionamiento_de_se-al_ESP32
Sistema de monitoreo de presión digital de 0-10 Bar, con acondicionamiento de señal de bucle de corriente ( 4-20mA) a tension (0-3,3V)para un ESP32-S3. calibración por software mediante regresión lineal y visualización en tiempo real con interfaz TFT.  framework Pioarduino (PlatformIO).


# Medidor de Presión Industrial (ESP32-S3)

Proyecto de medición de presión utilizando un sensor de 4-20mA, con interfaz gráfica en TFT_eSPI y sistema de calibración por software.

## Características
- **Hardware:** ESP32-S3 DevKit.
- **Sensor:** Presión 0-10 Bar (4-20mA).
- **Interfaz:** Menús interactivos, gráficas en tiempo real y modo oscuro.
- **Protección:** Circuito protegido con diodo Zener de 3.3V y filtro RC.

## Requisitos
- [PlatformIO](https://platformio.org/)
- Librerías: `TFT_eSPI`, `Adafruit NeoPixel`.

## Instalación
1. Clonar el repositorio.
2. Abrir con VS Code + PlatformIO.
3. El archivo `platformio.ini` descargará las dependencias automáticamente.
