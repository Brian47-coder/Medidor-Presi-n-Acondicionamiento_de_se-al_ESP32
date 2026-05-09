# Medidor de Presión Industrial v1.0
> Instrumentación Avanzada con ESP32-S3 & PlatformIO

![ESP32-S3](https://img.shields.io/badge/Hardware-ESP32--S3-blue)
![Arduino Framework](https://img.shields.io/badge/Framework-Arduino-orange)
![4-20mA](https://img.shields.io/badge/Sensor-4--20mA-green)

## 1. Descripción del Proyecto
Este sistema representa una solución de instrumentación digital diseñada para la monitorización precisa de presión en entornos industriales. Utiliza un sensor de presión de dos hilos con salida de corriente (4-20mA), procesada mediante un núcleo **ESP32-S3** para ofrecer visualización en tiempo real, calibración dinámica y una interfaz de usuario profesional.

> [!NOTE]
> **💡 NOTA:** Este proyecto ha sido desarrollado bajo el marco de la catedra de Electronica II en la Universidad Nacional del Nordeste (UNNE).

## 2. Hardware y Componentes

| Componente | Especificación | Función |
| :--- | :--- | :--- |
| Microcontrolador | ESP32-S3 DevKitC-1 n16-r8 | Procesamiento dual-core y gestión de ADC. |
| Sensor de Presión | Transmisor 4-20mA (0-10 Bar) | Medición de la variable física. |
| Pantalla | TFT LCD (ST7789) 240x320 | Interfaz gráfica de usuario (GUI). |
| Resistencia de Carga | 148Ω | Conversión de corriente a tensión (I-V). |
| Protección | Zener 3.3V | Seguridad del pin ADC contra sobretensiones. |

## 3. Arquitectura de Programación
El firmware se basa en una arquitectura de **Máquina de Estados Finitos (FSM)**, lo que permite una navegación fluida entre menús sin bloquear el proceso de lectura del sensor.

### Flujo Principal:
* **Capa de Adquisición:** Lectura del ADC con sobremuestreo para reducir el ruido térmico.
* **Capa de Procesamiento:** Aplicación de una curva de calibración basada en regresión lineal para compensar errores del sensor.
* **Capa de Interfaz:** Gestión de menús interactivos mediante la librería `TFT_eSPI`.

```cpp
// Ejemplo de estructura de estados
enum Estados { MENU_RAIZ, MONITOREO, CONF_PANTALLA, CALIBRACION };
Estados estadoActual = MENU_RAIZ;
