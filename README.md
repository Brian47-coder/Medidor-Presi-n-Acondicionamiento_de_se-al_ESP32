# Medidor de Presión Industrial v1.0
> **Instrumentación Avanzada con ESP32-S3 y Acondicionamiento de Señal 4-20mA**

![ESP32-S3](https://img.shields.io/badge/Hardware-ESP32--S3-blue)
![Arduino Framework](https://img.shields.io/badge/Framework-Arduino-orange)
![4-20mA](https://img.shields.io/badge/Sensor-4--20mA-green)

## 1. Descripción del Proyecto
Este sistema representa una solución de instrumentación digital diseñada para la monitorización de presión en entornos industriales. Utiliza un sensor de presión de dos hilos con salida de corriente (4-20mA), procesado mediante una etapa de acondicionamiento de señal para ser interpretado por un microcontrolador **ESP32-S3-LCD-1.47B** para ofrecer visualización en tiempo real, calibración dinámica y una interfaz de usuario profesional.

> [!NOTE]
> **💡 NOTA:** Este proyecto ha sido desarrollado bajo el marco de la catedra de Electronica II en la Universidad Nacional del Nordeste (UNNE).

## 2. Fundamentos de Instrumentación

### El Bucle de Corriente (4-20mA)
En la industria, se prefiere el bucle de corriente sobre el de tensión porque la corriente es inmune a las caídas de tensión por la resistencia de los cables largos y tiene una alta inmunidad al ruido electromagnético. 

Para que nuestro microcontrolador pueda leer estos valores, implementamos una **conversión de bucle de corriente a bucle de tensión (I-V)**. El pin ADC (Convertidor Analógico-Digital) del ESP32-S3 opera en un rango de 0 a 3.3V, por lo que la señal debe ser escalada adecuadamente.


### Cálculos de la Etapa de Entrada
Para la conversión, se utiliza una resistencia en el pin del microcontrolador configurado como entrada ADC y GND del micro, como GND de la fuente de 24V que alimenta el sensor de presion. Los cálculos teóricos vs. reales son:
* **Corriente máxima ($I_{max}$):** 20 mA ($0.02 A$).
* **Tensión máxima deseada ($V_{out}$):** $\approx 3.0 V$ (para dejar un margen de seguridad bajo los 3.3V).
* **Resistencia Teórica:** $R = V / I = 3.0V / 0.02A = 150 \Omega$.
* **Valor Comercial Utilizado:** $150 \Omega$ (nominal).
* **Valor Medido (Multímetro):** **$148 \Omega$**.

> **Protección del ADC:** Se integra un **Diodo Zener de 3.0V** en paralelo con la resistencia para recortar cualquier transitorio que supere la tensión de seguridad del micro. Además, se añade un **capacitor cerámico 104 (100nF)** para filtrar ruidos de alta frecuencia y estabilizar la lectura analógica antes del muestreo.

---

## 3. Hardware y Componentes

| Componente | Especificación | Función |
| :--- | :--- | :--- |
| **Microcontrolador** | ESP32-S3 DevKitC-1 n16-r8 | Procesamiento, ADC y controlador de pantalla integrado. |
| **Sensor de Presión** | Transmisor 4-20mA (0-10 Bar)(2 hilos)| Medición de la variable física. |
| **Pantalla** | TFT LCD (ST7789) 240x320 | Interfaz gráfica de usuario (GUI) y Visualización de datos. |
| **Resistencia** | 148Ω (medida) | Conversión de corriente a tensión (I-V). |
| **Protección** | Zener 3.0V | Seguridad del pin ADC contra sobretensiones. |
| **Protección** | Capacitor Ceramico 104 | Filtrar ruidos de alta frecuencia y estabilizar la lectura. |
| **Fuente CC** | 24V | Alimentacion del sensor/Trasmisor. |
| **Fuente CC** | 5V | Alimentacion del Microcontrolador. |
| **Botones** | 4 de Navegación + 1 de Zero | Control de UI y corrección de Offset. |


---

## 💻 4. Entorno de Programación

### El paso de PlatformIO a **Pioarduino**
Originalmente, el proyecto se gestionó en **VS Code con PlatformIO**. Sin embargo, debido a la velocidad de lanzamiento del hardware ESP32-S3, los mantenedores oficiales de PlatformIO a menudo tardan en verificar y subir las nuevas placas. Para obtener soporte nativo del **ESP32-S3-LCD-1.47B**, se migró a **Pioarduino**, un *fork* optimizado que contiene las definiciones de placas actualizadas y un soporte más ágil para las últimas versiones del framework de Espressif.

### Configuración de la Pantalla con la libreria (TFT_eSPI)
Uno de los desafíos es poder configurar la librería gráfica. El driver **ST7789** de 1.47" no estaba correctamente mapeado para el S3 en las configuraciones por defecto (orientadas al S2). 
Se tuvo que:
1. Localizar el archivo `User_Setup_Select.h`.
2. Crear un archivo de configuración personalizado: `Setup72b_ESP32_S3_ST7789_172x320.h`.
3. Mapear manualmente los pines de control, la velocidad del bus SPI y los offsets de la memoria de video para centrar la imagen en el display.
4. Activar la selección automática para nuestro archivo en `User_Setup_Select.h`.
---

## ⚙️ 5. Lógica de Procesamiento y Algoritmos

### Mapeo de Información (Bitmin / Bitmax)
El ADC del ESP32-S3 es de 12 bits ($0$ a $4095$). Para transformar estos "pasos" en presión real, se utiliza una función de mapeo lineal:
$$Presión = \frac{(LecturaADC - ADC_{min}) \times (Presión_{max} - Presión_{min})}{ADC_{max} - ADC_{min}}$$
Se definieron `bitmin` y `bitmax` para ajustar los límites del sensor, permitiendo ignorar lecturas erróneas fuera del rango de 4-20mA.

### Tratamiento de Señal: Buffer Circular
Para evitar las fluctuaciones en la pantalla, se implementó un **Promedio Móvil mediante un Buffer Circular**:
* El sistema guarda las últimas $N$ muestras.
* Calcula el promedio en cada ciclo.
* Se puede habilitar/deshabilitar por software para comparar la respuesta en tiempo real vs. la suavizada.

### Calibración y Zona Muerta
* **Corrección de Offset (Zero):** Mediante un botón físico dedicado, el software toma la lectura actual (con presión atmosférica) y la establece como el nuevo "cero".
* **Zona Muerta:** Un umbral configurable para que fluctuaciones mínimas cerca del cero no activen lecturas falsas.
* **Ganancia:** Factor multiplicativo para ajustar la escala si el sensor.

---

## 🎨 6. Interfaz Gráfica y Assets

### Renderizado de Imágenes (Hexadecimal RGB565)
Para mostrar el logo de la facultad (UNNE), se utilizó un proceso de conversión personalizado:
1. Una imagen original (JPG/PNG) se procesa mediante un script de **Python**.
2. El script transforma cada píxel a formato **RGB565** (16 bits de color).
3. El resultado es una matriz hexadecimal embebida en el código fuente, permitiendo un renderizado ultra rápido sin necesidad de tarjeta SD.

### Arquitectura por Máquina de Estados Finitos (FSM)
El firmware se basa en una arquitectura de **Máquina de Estados Finitos (FSM)**, lo que permite una navegación fluida entre menús sin bloquear el proceso de lectura del sensor.

**Ventajas:** * Multitarea fluida.
* Respuesta inmediata a los botones.
* Código escalable y fácil de depurar.

* **Estados (Gestión de Menú):** Controla el flujo de navegación principal, permitiendo alternar entre la pantalla de inicio, el árbol de configuración y las herramientas de ajuste.
* **ModoVisual (Renderizado):** Define cómo se presentan los datos en el display (Modo Numérico vs. Modo Graficadora en tiempo real).
* **UnidadMedida (Escalado):** Gestiona la conversión dinámica de la señal para mostrar la presión en diferentes unidades industriales (Bar, PSI, kPa, etc.).
* **OscEstado (Osciloscopio/Graficadora):** Controla el comportamiento del buffer de la gráfica, permitiendo congelar la señal o ajustar la base de tiempo.
* **EstadoCalibracion (Profundidad):** Un sub-estado crítico que gestiona el proceso de captura de puntos para la regresión lineal, el ajuste del offset y la validación de la ganancia.

### Flujo Principal:
* **Capa de Adquisición:** Lectura del ADC con sobremuestreo para reducir el ruido.
* **Capa de Procesamiento:** Aplicación de una curva de calibración basada en regresión lineal para compensar errores del sensor.
* **Capa de Interfaz:** Gestión de menús interactivos.


---

## 📐 7. Estructura del Menú
```text
Menu Principal (Root) modo visual = digital 
 ├── 📊 Princiapal
       ├── ⚙️ Config. Datos
       │    ├── Offset (Habilitar/desabilitar)
       │    ├── Ganancia (Habilitar/desabilitar)
       │    ├── Promedio (Habilitar/desabilitar)
       |    ├── Bitmin (Ajuste de Zero)
       │    ├── Bitmaz (ajuste)
       │    ├── Muestras (ajuste)
       │    ├── Intervalo (ajuste)
       │    ├── Ganancia (ajuste)
       |    ├── Ofset (ajuste)
       │    ├── Zona muerta (Habilitar/desabilitar)
       ├── 🖥️ Config. Pantalla
       │    ├── 🌓 Tema (Claro/Oscuro)
       │    ├── 💡 Brillo (PWM Control)
       │    └── 🔄 Retroiluminacion.
       ├─  👁️‍🗨️ Visual
       |    ├── 📈 Graficadora (OscEstado) -> Visualización temporal de la señal
       |    ├── 🔢 Digital -> Valor numérico de gran formato
       |    ├── ⏲️ Analógico -> Simulación de manómetro de aguja
       |    └── 📋 Parámetros -> Datos técnicos (Bits, mA, Voltaje)
       ├── 🛠️ Calibración (EstadoCalibracion)
       │    ├── 
       │    ├── 
       │    └── 
       ├── ℹ️ Información
       │    └── Datos del Proyecto (UNNE)
       └── ❓ Ayuda
            └── Guía rápida de botones
