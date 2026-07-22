#include <Arduino.h>
#include <TFT_eSPI.h>
#include "logo_unne_display_horizontal.h" // Aquí está tu imagen de 172x320
#include "logo_espressif_2.h" // Aquí está tu imagen de 64x64
#include "Adafruit_NeoPixel.h"
#include "colores.h"


// --- Configuración de Hardware de la libreria Grafica ---
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft);
TFT_eSprite spr = TFT_eSprite(&tft);

//////////////////////////////////////////
//////////////////////////////////////////
// --- Definiciones de pines del micro ---
#define PIN_BT_MENU   11
#define PIN_BT_UP     10
#define PIN_BT_DOWN   9
#define PIN_BT_BACK   8
#define PIN_BT_OFFSET 5 // Pin GP5 para el botón de calibración
#define PIN_BT_SENSOR 4
#define PIN_BACKLIGHT 46
//////////////////////////////////////////
//////////////////////////////////////////


//////////////////////////////////////////
//////////////////////////////////////////
// --- Definiciones para la animación de carga ---
#define SPRITE_WIDTH  172 
#define SPRITE_HEIGHT 160 
#define LOGO_SIZE 64      
#define CIRCLE_RADIUS 50  
#define DOT_RADIUS 4      
#define NUM_DOTS 8      
#define DOT_RADIUS 4    
#define FINAL_RADIUS 50  // El radio de la vuelta
//////////////////////////////////////////
//////////////////////////////////////////


//////////////////////////////////////////
//////////////////////////////////////////
// --- Definiciones para el Led RGB
#define PIN_RETROILUM 38         // El GPIO del LED RGB integrado
#define NUM_PIXELS 1       // Generalmente traen solo uno
Adafruit_NeoPixel ledRGB(NUM_PIXELS, PIN_RETROILUM, NEO_GRB + NEO_KHZ800);
//////////////////////////////////////////
//////////////////////////////////////////


//////////////////////////////////////////
//////////////////////////////////////////
// --- Estados y Navegación ---
enum Estados { PRINCIPAL, MENU_RAIZ, CONF_PANTALLA, CONF_DATOS, UNIDADES, VISUAL, MONITOR, CALIBRACION, INFO, AYUDA };
Estados estadoActual = PRINCIPAL;

enum ModoVisual { V_ANALOGICO, V_DIGITAL, V_PARAMETROS, V_GRAFICADORA };
ModoVisual modoVista = V_DIGITAL;

enum UnidadMedida { U_BAR, U_PSI, U_PA, U_KGF, U_MCA, U_ATM };
UnidadMedida unidadActual = U_BAR;

// --- Estados y Variables del Osciloscopio ---
enum OscEstado { OSC_NORMAL, OSC_MENU, OSC_MOVIENDO_CURSOR };
OscEstado osc_est = OSC_NORMAL;

// --- Estados y Variables para Calibración (de 0 a 7, 8 puntos) ---
enum EstadoCalibracion { CAL_INICIO, CAL_MIDIENDO, CAL_CONFIRMAR, CAL_RESULTADO, CAL_MENU, CAL_SUBMENU };
EstadoCalibracion estCalib = CAL_INICIO;

// --- Colores Dinámicos ---
uint16_t C_FONDO, C_TEXTO, C_ACCENTO, C_RECUADRO, C_EDIT, C_ESCALA, C_SUBMENU, C_PMAX, C_PMIN, C_PROM;

//////////////////////////////////////////
//////////////////////////////////////////


bool modoMonitorActivo = false; // Controla si transmitimos por USB a la PC



// --- Definiciones para el Sensor ---
#define V_REF 3.3           // Voltaje real del pin 3.3V del ESP (ajustar si es necesario)
#define ADC_MAX 4095.0      // Resolución de 12 bits


#define MAX_MUESTRAS     200     // 1000 muestras × 100ms = 100 segundos
#define ZONA_MUERTA  20     // ±40 bits alrededor de 793 (4mA) → presión = 0



//////////////////////////////////////////
//////////////////////////////////////////
// --- Variables del Promedio para media móvil ---
float   buffer[MAX_MUESTRAS];           // Buffer circular para las muestras de presión
int     bufindex  = 0;              // Índice actual en el buffer
float   bufsuma   = 0;              // Suma acumulada de las muestras
bool    buflleno  = false;          // Indica si el buffer ya se llenó al menos una vez
//////////////////////////////////////////
//////////////////////////////////////////


/****************************************/


//////////////////////////////////////////
//////////////////////////////////////////
// --- Variables de Medición ---
int valorADC_prom;
int valorADC_crudo;
int valorADC_crudo_prom;
float presionActual = 0.0;
float corrienteActual = 0.0;
unsigned long ultimamuestra = 0;

// --- Variables de Configuración de datos --- 
int bitmin = 650, bitmax = 3794,  umbral = 400, resistencia = 148;   // umbral = 400
bool promedio = true, offsetOn = true, valoresNegativos = true;
// --- Definiciones de media movil ---
int muestras = 30, intervalo = 100;
bool zonaMuerta = false;

// --- Definiciones para el Offset ---
int offsetBits = 0;     // Aquí guardamos el valor del ADC en bits cuando esta habilitado el offsetOn

// --- Variables de Configuración de Pantalla ---
bool modoOscuro = true, retroIlum = false;
int brillo = 150;
//////////////////////////////////////////
//////////////////////////////////////////


//////////////////////////////////////////
//////////////////////////////////////////
// --- Variables de UI Principal ---
int offsetScroll = 0;   // Para animar la bajada de la pantalla
bool editando = false;  // Para saber si estamos navegando o cambiando un número
unsigned long tPresionado = 0; // Para la aceleración del botón de 1 a 10
int seleccion = 0, orientacion = 1;  // 1=Horiz, 0=Vert
//////////////////////////////////////////
//////////////////////////////////////////


////////////////////////////////////////
////////////////////////////////////////
// --- Variables para la Graficadora ---
#define ANCHO_OSC 260
#define ALTO_OSC 80
float osc_buffer[ANCHO_OSC];
int osc_idx = 0;

// --- Variables para submenu de la Graficadora
bool osc_hold = false;          // Congelar pantalla
bool osc_puntos = false;        // Falso = Línea continua, Verdadero = Puntos
bool osc_linea_roja = true;     // Mostrar barrido
bool osc_cursor_en = false;     // Mostrar línea de cursor horizontal
int osc_cursor_pixel = ALTO_OSC / 2; // Posición Y del cursor
int osc_menu_sel = 0;           // Índice del menú de configuración

// --- Variables de UI de la Graficadora ---
int offsetScroll_osc = 0;   // Para animar la bajada de la pantalla
bool editando_osc = false; // Para saber si estamos navegando o cambiando un número
unsigned long tPresionado_osc = 0; // Para la aceleración del botón de 1 a 10

////////////////////////////////////////
////////////////////////////////////////



////////////////////////////////////////
////////////////////////////////////////
// --- Variables Globales para calibracion ---
const int CAL_MIN_PUNTOS = 2;
const int CAL_MAX_PUNTOS = 8;
float cal_teoricos[CAL_MAX_PUNTOS] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};
float cal_medidos[CAL_MAX_PUNTOS];
int cal_cantidad_puntos = CAL_MIN_PUNTOS;
int cal_punto_actual = 0;
int cal_seleccion = 0;
int cal_seleccion_sub = 0;

// --- Variables para submenu de calibracion

int cal_menu_sel = 0;           // Índice del menú de configuración

// --- Variables de UI de la calibracion ---
int offsetScroll_cal = 0;   // Para animar la bajada de la pantalla
bool editando_cal = false; // Para saber si estamos navegando o cambiando un número
unsigned long tPresionado_cal = 0; // Para la aceleración del botón de 1 a 10




////////////////////////////
///////////////////////////
//////////////////////////
bool ganancia = false;
float factorGanancia = 1.22; 
float factorOffset = 0.0;

float factorGanancia_sinconfirmar = 0.0;
float factorOffset_sinconfirmar =0.0;
//////////////////////////
///////////////////////////
////////////////////////////




// --- Para la funcion matematica de Regresion ---
float r_sumX = 0, r_sumY = 0, r_sumXY = 0, r_sumX2 = 0; // Para la tabla
float r_m = 1.0, r_b = 0.0;                             // Resultados temporales
float p_capturada_temp = 0.0;                           // Presión en el instante del OK
float p_capturada_temp_bar = 0.0;                       // Presión capturada en bar para guardar el punto
bool mostrarRegresion = false;                          // Bandera para graficar la nueva recta
bool mostrarRegresionCorregida = false;  
bool modoAscenso = true;                                // Control de dirección
//////////////////////////////////////////
//////////////////////////////////////////



void actualizarColores() {
    if (modoOscuro) {
        C_FONDO = TFT_BLACK; 
        C_TEXTO = TFT_WHITE; 
        C_ACCENTO = 0x07FF; // Cian
        C_RECUADRO = 0x2104; // Gris oscuro
        C_EDIT = TFT_ORANGE; // Color cuando entramos a editar un numero
        C_ESCALA = TFT_DARKGREY;
        C_SUBMENU = MAGENTA;
        C_PMAX = 0xFD20;
        C_PMIN = 0x07E0;
        C_PROM = 0xD6BA; //Gris claro
    } else {
        C_FONDO = 0xFFFF; // Blanco
        C_TEXTO = TFT_BLACK; 
        C_ACCENTO = TFT_BLUE; 
        C_RECUADRO = 0xCE79; // Gris claro
        C_EDIT = TFT_ORANGE;
        C_ESCALA = TFT_DARKGREY;
        C_SUBMENU = CORAL;
        C_PMAX = 0xF800;
        C_PMIN = 0x03E0;
        C_PROM = 0x4208; //Gris Oscuro
    }

/* 1. Contraste sobre fondo Blanco (TFT_WHITE)
Sobre blanco, los colores deben ser oscuros y saturados. 
Evita los amarillos o verdes claros, ya que se "pierden" con el brillo de la pantalla.
Azul Marino (0x000F): Es el más profesional para textos largos.  
Rojo Intenso (0xF800): Ideal para alarmas o valores $P_{max}$.  
Verde Bosque (0x03E0): Perfecto para estados "OK" o "RUN".  
Gris Carbón (0x4208): Úsalo para las líneas de la grilla del osciloscopio si el fondo es blanco.  

2. Contraste sobre fondo Negro (TFT_BLACK)Sobre negro, los colores deben ser brillantes (tipo neón) o tonos pastel saturados para que "salten" a la vista.
Amarillo Eléctrico (0xFFE0): El mejor para resaltar el valor de presión actual o el cursor.  
Cian / Celeste (0x07FF): Muy legible para etiquetas de unidades (bar, PSI).  
Verde Lima (0x07E0): El estándar para la traza del osciloscopio, emulando los equipos antiguos.  
Naranja (0xFD20): Excelente para advertencias intermedias o el valor de $P_{prom}$.  
Magenta / Fucsia (0xF81F): Úsalo para elementos secundarios como la base de tiempo.   */

}


// --- Función de mapeo para decimales ---
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
void setup() {
    Serial.begin(115200);

     // 1. --- Configuración de pines ---
    pinMode(PIN_BT_OFFSET, INPUT_PULLUP);            // Botón conectado entre GP5 y GND
    pinMode(PIN_BT_MENU, INPUT_PULLUP);             // Botón conectado entre GP10 y GND        
    pinMode(PIN_BT_UP, INPUT_PULLUP);               // Botón conectado entre GP9 y GND
    pinMode(PIN_BT_DOWN, INPUT_PULLUP);             // Botón conectado entre GP8 y GND
    pinMode(PIN_BT_BACK, INPUT_PULLUP);             // Botón conectado entre GP7 y GND
    pinMode(PIN_BACKLIGHT, OUTPUT); analogWrite(PIN_BACKLIGHT, brillo);


    // 2. --- Inicializacion del LED RGB integrado ---
    ledRGB.begin();
    ledRGB.setBrightness(150); // No lo pongas al máximo para no encandilar, el maximo real es 255 pero con 150 ya se ve bien
    ledRGB.setPixelColor(0, ledRGB.Color(0, 0, 0));     // Estado encendido / Activo (Verde)
    ledRGB.show(); // Asegura que el LED se actualice con el brillo inicial


    // 3. Iniciar pantalla
    tft.init();
    tft.invertDisplay(true);
    tft.fillScreen(TFT_BLACK);

    // 4. --- PRIMER INICIO: Pelotitas + Logo Espressif ---
    tft.setRotation(0);                             // Vertical para la animación original
    spr.createSprite(SPRITE_WIDTH, SPRITE_HEIGHT);
    unsigned long startTime = millis();             // Capturas el "tiempo actual" justo antes de empezar
    float angle = 0;
    float radii[NUM_DOTS] = {0};                    // Radios iniciales de las pelotitas empiezan en 0 todas.
    while (millis() - startTime < 1000) {           // Dura 3 segundo
        
        spr.fillSprite(TFT_BLACK);

        // Calculo del centro de las coordenadas del sprite, variables locales.
        int centerX = SPRITE_WIDTH / 2;
        int centerY = SPRITE_HEIGHT / 2;

        // Dibujar Logo Espressif en el centro del sprite
        spr.setSwapBytes(false);                    // Define si se intercambian los bytes de los colores 16-bit
        spr.pushImage(centerX - 32, centerY - 32, 64, 64, (uint16_t*)logo_espressif_2_bin);

        /*Tipo: Es un método de TFT_eSprite  
        Propósito: Dibuja una imagen (bitmap) en el sprite
        Parámetro 1 (centerX - 32): Posición X = 86 - 32 = 54 píxeles desde la izquierda
        Parámetro 2 (centerY - 32): Posición Y = 80 - 32 = 48 píxeles desde arriba (centra la imagen)
        Parámetro 3 (64): Ancho de la imagen en píxeles
        Parámetro 4 (64): Altura de la imagen en píxeles
        Parámetro 5 (uint16_t*)logo_espressif_bin: Puntero a los datos de la imagen en formato 16-bit, viene del archivo logo_espressif.h
        */
        
        // Lógica de las pelotitas
        for (int i = 0; i < NUM_DOTS; i++) {
            float dotAngle = angle + (i * 2 * PI / NUM_DOTS);      // Ángulo específico de cada pelotita, se distribuyen uniformemente alrededor del círculo
            
            // --- LÓGICA DE SALIDA INDEPENDIENTE DEL ÁNGULO FINAL ---
            // Cada pelotita tiene un retraso según su índice (i * 0.8)
            // Solo empieza a salir si el ángulo global superó su turno
            float startThreshold = i * (2 * PI / NUM_DOTS);
            
            if (angle > startThreshold && angle < 4 * PI) {         // Si el ángulo global superó el turno de esta pelotita y no llegó a las 2 vueltas, empieza a crecer
            // Mientras no llegue al radio final, sigue creciendo
                if (radii[i] < FINAL_RADIUS) radii[i] += 2.0; 
            }
            // --- LÓGICA DE GUARDADO (En la vuelta 3) ---
            else if (angle > 4 * PI) {
                float stopThreshold = (4 * PI) + (i * 2 * PI / NUM_DOTS);
                if (angle > stopThreshold) {
                    if (radii[i] > 0) radii[i] -= 2.0; 
                }
            }
            // --- Dibujar la pelotita solo si su radio es mayor a 0 ---
            if (radii[i] > 0) {
                int dotX = centerX + cos(dotAngle) * radii[i];
                int dotY = centerY + sin(dotAngle) * radii[i];
                spr.fillCircle(dotX, dotY, DOT_RADIUS, TFT_WHITE);
            }
        }
        // --- Sprite completo a la pantalla física ---
        spr.pushSprite(0, (320 - SPRITE_HEIGHT) / 2);

        /* Tipo: Método de TFT_eSprite
        Propósito: Envía el buffer del sprite a la pantalla física
        Parámetro 1 (0): Posición X en la pantalla = 0 (sin desplazamiento horizontal)
        Parámetro 2 (320 - SPRITE_HEIGHT) / 2: Posición Y = (320 - 160) / 2 = 80 píxeles (centra verticalmente en la pantalla)
        Efecto: Muestra todo lo que dibujamos en el sprite en la pantalla física
        */

        angle += 0.11;                              // Velocidad de rotación, ángulo base global (aumenta cada frame)

        // --- Reiniciar ciclo completo después de las 2 vueltas (4*PI) ---
        if (angle >= 6 * PI) {
            angle = 0;                              // inicia el ángulo a cero

            for(int i=0; i<NUM_DOTS; i++){
                radii[i] = 0;                       // Reinicia los radios de todas las pelotitas a 0
            }
            delay(50);                              // Pausa antes de empezar de nuevo
        }
        
        delay(10); 

    }

    spr.deleteSprite();                             // Liberamos RAM

    // 5. --- SEGUNDO INICIO: Logo de la Facultad ---
    tft.setRotation(1);                             // Cambiamos a Horizontal para el logo de la UNNE
    tft.fillScreen(TFT_BLACK);
    tft.setSwapBytes(false);
    tft.pushImage(0, 0, 320, 172, (uint16_t*)imagen_total);   // Dibujo del logo de la facultad el de 320x172
    delay(1000);                                    // Dura 1 segundo

    // 6. --- FINALIZACIÓN DEL INICIO ---
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(1);                             // Dejamos la pantalla lista para la medición de presión

    
    // 7. --- Inicializa el buffer en el valor de la base (ALTO_OSC) ---
    for (int i = 0; i < ANCHO_OSC; i++) {
        osc_buffer[i] = ALTO_OSC + 100; 
    }
    osc_idx = 0; // Reinicia el puntero de barrido


    tft.init();
    tft.setRotation(orientacion);
    canvas.createSprite(320, 172);
    actualizarColores();
}
///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////








///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// --- Conversiones Matemáticas ---
String getUnidadStr() {
    switch(unidadActual) {
        case U_BAR: return "Bar"; 
        case U_PSI: return "Psi"; 
        case U_PA: return "Pa";
        case U_KGF: return "kgf/cm²"; 
        case U_MCA: return "m.c.a"; 
        case U_ATM: return "Atm";
    } return "";
}

float getPresionConvertida() {
    switch(unidadActual) {
        case U_BAR: return presionActual;
        case U_PSI: return presionActual * 14.5038;
        case U_PA:  return presionActual * 100000.0;
        case U_KGF: return presionActual * 1.01972;
        case U_MCA: return presionActual * 10.197;
        case U_ATM: return presionActual * 0.986923;
    } return presionActual;
}

float convertirPresionDesdeBar(float presionBar) {
    switch(unidadActual) {
        case U_BAR: return presionBar;
        case U_PSI: return presionBar * 14.5038;
        case U_PA:  return presionBar * 100000.0;
        case U_KGF: return presionBar * 1.01972;
        case U_MCA: return presionBar * 10.197;
        case U_ATM: return presionBar * 0.986923;
    } return presionBar;
}

float getMaxEscala() {
    switch(unidadActual) {
        case U_BAR: return 10.0;
        case U_PSI: return 145.04;
        case U_PA:  return 1000000.0; // 1 MPa
        case U_KGF: return 10.197;
        case U_MCA: return 101.97;
        case U_ATM: return 9.87;
    } return 10.0;
}
///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////







///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// --- Componentes de Interfaz ---
void dibujarHeader(const char* titulo) {
    canvas.fillSprite(C_FONDO);
    canvas.fillRoundRect(5, 5, 310, 30, 5, C_RECUADRO);
    canvas.setTextColor(C_ACCENTO);
    canvas.drawCentreString(titulo, 160, 12, 2);
}

void dibujarMenu(const char* titulo, String opciones[], int total) {
    dibujarHeader(titulo);
    
    int itemsVisibles = 4;
    int altoBox = 26;       // Aumentado para que no corte letras
    int espaciado = 30;     // Espacio total entre cajas
    int yInicial = 42;      // Donde arranca la primera caja

    // Lógica de Scroll dinámico
    if (seleccion < offsetScroll) offsetScroll = seleccion;
    if (seleccion >= offsetScroll + itemsVisibles) offsetScroll = seleccion - itemsVisibles + 1;

    for (int i = offsetScroll; i < min(total, offsetScroll + itemsVisibles); i++) {
        int indexVisual = i - offsetScroll;
        int yPos = yInicial + (indexVisual * espaciado);
        
        int ancho = (i == seleccion) ? 280 : 260;
        int alto = (i == seleccion) ? altoBox + 4 : altoBox; 
        int xPos = (i == seleccion) ? 15 : 25;
        
        // Si está seleccionado y editando, cambia a Naranja
        uint16_t colorBox = (i == seleccion) ? (editando ? C_EDIT : C_ACCENTO) : C_RECUADRO;
        uint16_t colorTxt = (i == seleccion) ? (modoOscuro ? TFT_BLACK : TFT_WHITE) : C_TEXTO;

        canvas.fillRoundRect(xPos, yPos, ancho, alto, 4, colorBox);
        canvas.setTextColor(colorTxt);
        // Ajuste milimétrico para que la letra quede en el centro de la caja
        canvas.drawString(opciones[i], xPos + 10, yPos + (alto / 2) - 8, 2);
    }

    // Barra de Progreso dinámica
    int alturaBarra = 120 / total;
    canvas.fillRoundRect(305, 42 + (seleccion * alturaBarra), 6, alturaBarra, 3, C_ACCENTO);
}

void modificarValorConfig(int sel, int incremento) {
    if (sel == 0) bitmin = constrain(bitmin + incremento, 0, 999);
    else if (sel == 1)  bitmax = constrain(bitmax + incremento, 1000, 4095);
    else if (sel == 2) {
        muestras = constrain(muestras + incremento, 1, MAX_MUESTRAS);
        // IMPORTANTE: Reiniciar el buffer al cambiar el tamaño para evitar basura
        bufindex = 0;
        buflleno = false;
        for(int i=0; i<MAX_MUESTRAS; i++) buffer[i] = 0;
    }
    else if (sel == 3) intervalo = constrain(intervalo + incremento, 1, 100000);
    else if (sel == 4) umbral = constrain(umbral + incremento, 1, MAX_MUESTRAS);
    else if (sel == 5) resistencia = constrain(resistencia + incremento, 1, 300);
    else if (sel == 11) factorGanancia = constrain(factorGanancia + incremento, 1.0, 10.0);
    else if (sel == 12) factorOffset = constrain(factorOffset + incremento, 0, 1000);
}

void configurarPuntosTeoricos();

void modificarValorMenuCal(int sel, int incremento) {
    if (sel == 1) factorOffset = constrain(factorOffset + incremento, 0, 1000);
    else if (sel == 3) factorGanancia = constrain(factorGanancia + incremento, 1.0, 10.0);
    else if (sel == 5) {
        int nuevaCantidad = constrain(cal_cantidad_puntos + incremento, CAL_MIN_PUNTOS, CAL_MAX_PUNTOS);
        if (nuevaCantidad != cal_cantidad_puntos) {
            cal_cantidad_puntos = nuevaCantidad;
            cal_punto_actual = 0;
            mostrarRegresion = false;
            mostrarRegresionCorregida = false;
            configurarPuntosTeoricos();
        }
    }
}

void dibujarBarraParametro(int y, String titulo, String valor, float porcentaje) {
    canvas.setTextColor(C_TEXTO);
    canvas.drawString(titulo, 10, y, 2);
    canvas.setTextColor(C_ACCENTO);
    canvas.drawRightString(valor, 310, y, 2);
    
    // Contenedor de barra
    canvas.drawRoundRect(10, y + 16, 300, 10, 3, C_RECUADRO);
    // Relleno de barra (limitado a 0-100%)
    if (porcentaje < 0) porcentaje = 0; if (porcentaje > 1) porcentaje = 1;
    canvas.fillRoundRect(12, y + 18, 296 * porcentaje, 6, 2, C_ACCENTO);
}
///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// --- Componentes de Interfaz Graficadora ---
void dibujarOsciloscopio(int cx, int cy) {
    
    // Variables estáticas para recordar los picos entre cada frame
    static float p_max = -999.0;
    static float p_min = 999.0;
    static unsigned long ultimoAvance_osc = 0; // Variable para controlar el tiempo del barrido
    
    float maxEscala = getMaxEscala();
    static float p_actual;

    // 1. Lógica de retención (Hold) y Picos
    if (!osc_hold) {
        if (millis() - ultimoAvance_osc >= intervalo) {
            ultimoAvance_osc = millis(); // Resetear el cronómetro del barrido

            p_actual = getPresionConvertida(); // Tu valor ya filtrado (P_prom)
    
            // --- Lógica de Picos (Min/Max) ---
            // Si el índice vuelve a 0 (nuevo barrido), reseteamos los picos
            if (osc_idx == 0) {
                p_max = p_actual;
                p_min = p_actual;
            } else {
                if (p_actual > p_max) p_max = p_actual;
                if (p_actual < p_min) p_min = p_actual;
            }
            float pSegura = constrain(p_actual, 0.0, maxEscala);
            osc_buffer[osc_idx] = mapFloat(pSegura, 0.0, maxEscala, ALTO_OSC, 0); 
            osc_idx = (osc_idx + 1) % ANCHO_OSC; // Avanzar el puntero
        }
    }

    // 2. Dibujar Marco y Fondo del Osciloscopio
    canvas.fillRect(cx, cy, ANCHO_OSC, ALTO_OSC, TFT_BLACK);
    canvas.drawRect(cx - 1, cy - 1, ANCHO_OSC + 2, ALTO_OSC + 2, C_RECUADRO);

    // 3. Dibujar Grilla (Retícula)
    canvas.setTextColor(C_ESCALA);
    for (int i = 0; i <= ALTO_OSC; i += ALTO_OSC/5) {    /////////////////////////era 4
        // Líneas horizontales punteadas o débiles
        for (int j = 0; j < ANCHO_OSC; j+=5) canvas.drawPixel(cx + j, cy + i, C_RECUADRO);
    }

    // 4. Trazado de la Onda (Corregido de Izquierda a Derecha)
    // Buscamos el "hueco" donde está escribiendo la línea roja para no unir el punto viejo con el nuevo
    int gap = osc_idx - 1;
    if (gap < 0) gap = ANCHO_OSC - 1;

    for (int i = 0; i < ANCHO_OSC - 1; i++) {
        if (i != gap) { // Si no estamos en el punto de corte del barrido
            if (osc_puntos) {
                canvas.drawPixel(cx + i, cy + osc_buffer[i], TFT_GREEN);
            } else {
                canvas.drawLine(cx + i, cy + osc_buffer[i], cx + i + 1, cy + osc_buffer[i+1], TFT_GREEN);
            }
        }
    }

    // 5. Elementos Dinámicos: Línea Roja y Cursor
    if (osc_linea_roja) {
        canvas.drawLine(cx + osc_idx, cy, cx + osc_idx, cy + ALTO_OSC, TFT_RED); 
    }

    if (osc_cursor_en || osc_est == OSC_MOVIENDO_CURSOR) {
        uint16_t colorCursor = (osc_est == OSC_MOVIENDO_CURSOR) ? TFT_YELLOW : C_RECUADRO;
        canvas.drawLine(cx, cy + osc_cursor_pixel, cx + ANCHO_OSC, cy + osc_cursor_pixel, colorCursor);
        
        // Calcular presión en la posición del cursor (Mapeo inverso)
        float valCursor = mapFloat(ALTO_OSC - osc_cursor_pixel, 0, ALTO_OSC, 0.0, maxEscala);
        canvas.setTextColor(colorCursor);
        canvas.drawString(String(valCursor, 2), cx + 5, cy + osc_cursor_pixel - 12, 1);
    }

    // 6. Estadísticas Inferiores y Base de Tiempo
    //int yFila1 = cy + ALTO_OSC + 8;
    // Coordenadas Y para las dos filas de texto debajo del gráfico
    int yFila1 = cy + ALTO_OSC + 10 + 10;
    int yFila2 = yFila1 + 18 - 5; 

    canvas.setTextColor(C_PROM);
    canvas.drawString("P_prom: " + String(p_actual, 2), cx, yFila1, 1);
    canvas.drawRect(cx+90, yFila1 -4 , 40, 30 + 1, (osc_hold ? TFT_RED : TFT_GREEN));
    canvas.setTextColor(osc_hold ? TFT_RED : TFT_GREEN);
    canvas.drawString(osc_hold ? "HOLD" : "RUN", cx + 100 -2 , yFila1 - 4, 2);
    canvas.drawString(osc_hold ? "<" : ">", cx + 100 + 8 -1, yFila1 + 10, 2);
    
    // Base de tiempo e intervalo
    float t_total_s = (ANCHO_OSC * intervalo) / 1000.0; // Tiempo total = (ANCHO_OSC * intervalo) / 1000.0
    canvas.setTextColor(C_ESCALA);
    canvas.drawString("T.Base: " + String(t_total_s, 1) + "s (dt:" + String(intervalo) + "ms)", cx + 60, yFila1-20 +5, 1);

    // Botón Virtual "Config" a la derecha
    canvas.setTextColor(C_ACCENTO);
    canvas.drawRect(cx + ANCHO_OSC - 25, cy + ALTO_OSC + 5, 45, 18, C_ACCENTO);
    canvas.drawString("Config", cx + ANCHO_OSC - 20, cy + ALTO_OSC + 8+2, 1);

    canvas.drawRect(cx + ANCHO_OSC - 25, cy + ALTO_OSC + 5 + 18 + 5, 45, 18, C_ACCENTO);
    canvas.drawString("Back", cx + ANCHO_OSC - 20 +6, cy + ALTO_OSC + 8 + 18 +5+2, 1);


    // 6. Submenú Flotante de Configuración
    if (osc_est == OSC_MENU) {
        int mx = cx + ANCHO_OSC - 90;
        int my = cy + 10;
        canvas.fillRoundRect(mx, my, 105, 70, 3, TFT_DARKGREY);
        canvas.drawRoundRect(mx, my, 105, 70, 3, C_TEXTO);
        
        canvas.setTextColor(C_TEXTO);
        canvas.drawString(osc_menu_sel == 0 ? "> Trazo: " + String(osc_puntos?"Ptos":"Lin") : "  Trazo: " + String(osc_puntos?"Ptos":"Lin"), mx + 5, my + 5, 1);
        canvas.drawString(osc_menu_sel == 1 ? "> L.Roja: " + String(osc_linea_roja?"Si":"No") : "  L.Roja: " + String(osc_linea_roja?"Si":"No"), mx + 5, my + 18, 1);
        canvas.drawString(osc_menu_sel == 2 ? "> Cursor Y" : "  Cursor Y", mx + 5, my + 31, 1);
        
        canvas.drawString(osc_menu_sel == 4 ? "> Salir" : "  Salir", mx + 5, my + 57, 1);
        canvas.setTextColor((editando_osc ? C_EDIT : C_TEXTO));
        canvas.drawString(osc_menu_sel == 3 ? "> Inter.: " + String(intervalo) + " ms":"  Inter.: " + String(intervalo) + " ms", mx + 5, my + 44, 1);

    }

    // Escala del Eje Y
    canvas.setTextColor(C_ESCALA);
    canvas.drawString(String(maxEscala, 1), cx - 25, cy, 1);
    canvas.drawString("0", cx - 15, cy + ALTO_OSC - 8, 1);

    
    canvas.setTextColor(C_PROM);
    // Lo alineamos a la derecha usando cx + 140
    canvas.drawString("Uni: " + getUnidadStr(), cx + 140, yFila1, 1); 

    // Fila 2: Máximos y Mínimos con colores para identificarlos rápido
    canvas.setTextColor(C_PMAX); 
    canvas.drawString("P_max: " + String(p_max, 2), cx, yFila2, 1);
    
    canvas.setTextColor(C_PMAX);
    canvas.drawString("P_min: " + String(p_min, 2), cx + 140, yFila2, 1);
 

}
///////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////






//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
// --- logica de Interfaz Principal ---
void pantallaPrincipal() {
    canvas.fillSprite(C_FONDO);
    
    canvas.drawRect(5, 5, 35, 25, C_ACCENTO);
    canvas.setTextColor(C_ACCENTO);
    canvas.drawString("MENU", 10, 14, 1);
    if (offsetOn) 
        canvas.fillCircle(300, 15, 5, TFT_ORANGE);
    if (ganancia) 
        canvas.fillCircle(275, 15, 5, TFT_ORANGE);
    if (zonaMuerta) 
        canvas.fillCircle(250, 15, 5, TFT_ORANGE);


    // Circulos en la principal

    canvas.setTextColor(TFT_ORANGE);
    canvas.drawCircle(300, 15, 8, TFT_ORANGE);
    canvas.drawString("Offs.", 300-10, 30-2, 1);
    canvas.drawCircle(275, 15, 8, TFT_ORANGE);
    canvas.drawString("Gan", 275-8, 30-2, 1);
    canvas.drawCircle(250, 15, 8, TFT_ORANGE);
    canvas.drawString("Z.M", 250-10, 30-2, 1);

    // Recuadros de SUBmenu

    int comp = analogRead(PIN_BT_SENSOR);
    if (modoVista == V_DIGITAL && comp >= umbral) {
        canvas.drawRect(100, 5, 27, 25, C_SUBMENU); // Recuadro para la lectura digital, un poco más ancho para que no corte el texto, 5
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("DIG", 105, 14, 1);
        
        // canvas.drawRect(150, 5, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("ANA", 155, 14, 1);

        //canvas.drawRect(200, 5, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("PAR", 205, 14, 1);
    }

    if (modoVista == V_ANALOGICO && comp >= umbral) {
        //canvas.drawRect(5, 42, 27, 25, C_SUBMENU); 
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("DIG", 10, 51, 1);  
        
        canvas.drawRect(5, 72, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("ANA", 10, 81, 1);

        //canvas.drawRect(5, 102, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("PAR", 10, 111, 1);
    }

    if (modoVista == V_PARAMETROS && comp >= umbral) {
        //canvas.drawRect(100, 5, 27, 25, C_SUBMENU); 
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("DIG", 105, 14, 1);
        
        // canvas.drawRect(150, 5, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("ANA", 155, 14, 1);

        canvas.drawRect(200, 5, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("PAR", 205, 14, 1);
    }


    if (modoVista == V_GRAFICADORA && comp >= umbral) {
        //canvas.drawRect(100, 5, 27, 25, C_SUBMENU); 
        //canvas.setTextColor(C_SUBMENU);
        //canvas.drawString("DIG", 105, 14, 1);
        
        canvas.drawRect(110, 5, 100, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("GRAFICADORA", 125+4, 14, 1);

        //canvas.drawRect(200, 5, 27, 25, C_SUBMENU);
        //canvas.setTextColor(C_SUBMENU);
        //canvas.drawString("PAR", 205, 14, 1);
    }

    int raw = analogRead(PIN_BT_SENSOR);
    if (raw < umbral) {
        canvas.setTextColor(TFT_RED);
        canvas.drawCentreString("ADVERTENCIA", 160, 60-8, 4);
        canvas.drawCentreString("SENSOR DESCONECTADO", 160, 100-8, 4);
        canvas.drawCentreString("O CABLE ROTO", 160, 140-8, 4);
        canvas.drawRect(5, 40, 310, 125, TFT_RED);

        if (modoVista == V_GRAFICADORA) {
            //canvas.drawRect(100+50, 5, 100, 25, C_SUBMENU);
            canvas.setTextColor(C_SUBMENU);
            canvas.drawString("GRAFICADORA", 125+4, 14, 1);
        }
        else{
        //canvas.drawRect(100, 5, 27, 25, C_SUBMENU); 
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("DIG", 105, 14, 1);
        
        // canvas.drawRect(150, 5, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("ANA", 155, 14, 1);

        //canvas.drawRect(200, 5, 27, 25, C_SUBMENU);
        canvas.setTextColor(C_SUBMENU);
        canvas.drawString("PAR", 205, 14, 1);
        }

    } else {
        if (modoVista == V_DIGITAL) {
            canvas.setTextColor(C_TEXTO);
            canvas.drawCentreString(String(getPresionConvertida(), 2) + " " + getUnidadStr(), 160, 65, 4);
            canvas.setTextColor(C_ACCENTO);
            canvas.drawCentreString(String(corrienteActual, 1) + " mA", 160, 130, 4);
            
    } else if (modoVista == V_ANALOGICO) {
            int cx = 160, cy = 130; // Centro del manómetro un poco más abajo

            // Obtenemos el valor máximo según la unidad actual.
            float maxVal = getMaxEscala();
            
            // 1. Dibujar Escala Graduada de 0 a 180 grados
            // Recorremos de 180 a 0 para que el 0 bar esté a la izquierda
            for (int i = 0; i <= 10; i++) {
                // Calculamos el valor numérico que corresponde a esta rayita
                float valorRaya = (maxVal / 10.0) * i;


                // Mapeo: 0 bar = 180°, 8 bar = 0°
                float angulo = 180.0 - (i * 18.0); // 180 / 10 = 18 grados por división
                float rad = angulo * PI / 180.0;
                
                int x1 = cx + cos(rad) * 85; 
                int y1 = cy - sin(rad) * 85;
                int x2 = cx + cos(rad) * 105; 
                int y2 = cy - sin(rad) * 105;
                
                // Líneas de escala (Accent color)
                canvas.drawLine(x1, y1, x2, y2, C_ACCENTO);
                
                // Números de la escala
                canvas.setTextColor(C_ESCALA);
                int xTxt = cx + cos(rad) * 120;
                int yTxt = cy - sin(rad) * 120;

                // Si el número es muy grande (como en Pascales), usamos 0 decimales, 
                // si es chico (como Bar), usamos 1 para que se entienda mejor.
                String txtEscala = (maxVal > 50) ? String((int)valorRaya) : String(valorRaya, 1);

                canvas.drawCentreString(txtEscala , xTxt, yTxt - 5, 2);
            }

            // 2. Dibujar el arco de fondo decorativo
            canvas.drawArc(cx, cy, 105, 103, 90, 270, C_RECUADRO, C_FONDO);

            // 3. Lógica de la Aguja 
            float pSegura = constrain(getPresionConvertida(), 0.0, maxVal);
            float anguloAguja = 180.0 - ((pSegura / maxVal) * 180.0);  // se tiene que ver como se comporta
            float radAg = anguloAguja * PI / 180.0;
            
            // Coordenadas de la punta de la aguja
            int ax = cx + cos(radAg) * 95; 
            int ay = cy - sin(radAg) * 95;
            
            // Dibujamos la aguja como un triángulo alargado (estilo instrumental de avión)
            int lx = cx + cos(radAg + 0.15) * 15; 
            int ly = cy - sin(radAg + 0.15) * 15;
            int rx = cx + cos(radAg - 0.15) * 15; 
            int ry = cy - sin(radAg - 0.15) * 15;
            
            canvas.fillTriangle(ax, ay, lx, ly, rx, ry, TFT_RED);
            
            // 4. Centro del instrumento (Hub)
            canvas.fillCircle(cx, cy, 15, C_RECUADRO);
            canvas.drawCircle(cx, cy, 15, C_ACCENTO);
            canvas.fillCircle(cx, cy, 5, C_TEXTO); // Perno central

            // 5. Lectura digital inferior
            canvas.setTextColor(C_TEXTO);
            canvas.drawCentreString(String(getPresionConvertida(), 2) + " " + getUnidadStr(), cx, cy + 20, 4);
        } 
        else if (modoVista == V_PARAMETROS) {
            float tension = (corrienteActual / 1000.0) * resistencia;
            float promedio_presion = getPresionConvertida() / getMaxEscala(); 
            float promedio_corriente = (corrienteActual - 4.0) / 16.0; // Normalizamos la corriente entre 4mA (0%) y 20mA (100%)
            float promedio_tension = tension / V_REF; // Normalizamos la tensión entre 0V y 3,3V
            float promedio_adc = valorADC_prom / ADC_MAX; // Normalizamos el valor ADC entre 0 y 4095
            
            dibujarBarraParametro(35, "Presion", String(getPresionConvertida(), 2) + getUnidadStr(), promedio_presion);
            dibujarBarraParametro(65, "Corriente", String(corrienteActual, 1) + " mA", promedio_corriente);
            dibujarBarraParametro(95, "Tension (R : " + String(resistencia) + " Ohm)", String(tension, 2) + " V", promedio_tension);
            dibujarBarraParametro(125, "Senal ADC", String(valorADC_prom) + " Bits", promedio_adc);

            /* Ejemplo de llamada a la función para dibujar la barra de presión:
            dibujarBarraParametro(35, "Presion", "5.20 Bar", 0.52);

            35 (Coordenada Y): posición vertical donde se dibujará esa barra específica.35, 65, 95 y 125,dejando un espacio de 30 píxeles entre cada parámetro.

            "Presion" (Etiqueta): Es el nombre del parámetro que aparece a la izquierda.

            String(...) + getUnidadStr() (Valor): Es el texto que muestra el número actual con sus unidades (Bar, mA, V, etc.). a la derecha.

            presionActual / 10.0 (Porcentaje/Progreso): Es un valor entre 0.0 y 1.0. Esto le dice a la función qué tan "llena" debe estar la barra visualmente*/
        }
        else if (modoVista == V_GRAFICADORA) {
            dibujarOsciloscopio(30, 45); // Posición del osciloscopio dentro de la pantalla
            if (osc_est == OSC_NORMAL) {
            // Controles principales
                if (digitalRead(PIN_BT_UP) == LOW) { osc_hold = true; delay(200); }
                if (digitalRead(PIN_BT_DOWN) == LOW) { osc_hold = false; delay(200); }
                if (digitalRead(PIN_BT_MENU) == LOW) { osc_est = OSC_MENU; delay(200); }
                if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = MENU_RAIZ; delay(200); }
            }
            else if (osc_est == OSC_MENU) {
                if (!editando_osc) {
                    // Navegar normal del pequeño submenú
                    if (digitalRead(PIN_BT_UP) == LOW) { osc_menu_sel = max(0, osc_menu_sel - 1); delay(200); }
                    if (digitalRead(PIN_BT_DOWN) == LOW) { osc_menu_sel = min(4, osc_menu_sel + 1); delay(200); }
                    
                    if (digitalRead(PIN_BT_MENU) == LOW) {
                        if (osc_menu_sel == 0) osc_puntos = !osc_puntos; // Cambiar modo trazo
                        if (osc_menu_sel == 1) osc_linea_roja = !osc_linea_roja; // Toggle línea
                        if (osc_menu_sel == 2) { osc_cursor_en = true; 
                            osc_est = OSC_MOVIENDO_CURSOR; }
                        if (osc_menu_sel == 3) editando_osc = true; 
                        if (osc_menu_sel == 4) osc_est = OSC_NORMAL;
                        delay(200);
                    }
                    if (digitalRead(PIN_BT_BACK) == LOW) { osc_est = OSC_NORMAL; editando_osc = false; delay(200); }

                }else if (editando_osc) {
                    // Modo Edición (Cambiar valores de 1 en 1 o 10 en 10)
                    if (digitalRead(PIN_BT_UP) == LOW) {
                        if (tPresionado_osc == 0) tPresionado_osc = millis();
                        int inc = (millis() - tPresionado_osc > 600) ? 10 : 1; // Si pasa 600ms, sube de a 10
                        modificarValorConfig(osc_menu_sel, inc);
                        delay((inc == 10) ? 80 : 200); // Rápido si mantiene, normal si toca una vez
                    } 
                    else if (digitalRead(PIN_BT_DOWN) == LOW) {
                        if (tPresionado_osc == 0) tPresionado_osc = millis();
                        int inc = (millis() - tPresionado_osc > 600) ? 10 : 1;
                        modificarValorConfig(osc_menu_sel, -inc);
                        delay((inc == 10) ? 80 : 200);
                    } 
                    else {
                        tPresionado_osc = 0; // Se soltó el botón
                    }
                    
                    if (digitalRead(PIN_BT_MENU) == LOW || digitalRead(PIN_BT_BACK) == LOW) {
                        editando_osc = false; tPresionado_osc = 0; delay(200);
                    }
                }
                else {
                    if (digitalRead(PIN_BT_BACK) == LOW) { osc_est = OSC_NORMAL; editando_osc = false; delay(200); }
                }
            }
            else if (osc_est == OSC_MOVIENDO_CURSOR) {
                // Mover la línea amarilla del cursor (invertido porque Y=0 es arriba)
                if (digitalRead(PIN_BT_UP) == LOW) { osc_cursor_pixel = max(0, osc_cursor_pixel - 2); delay(50); }
                if (digitalRead(PIN_BT_DOWN) == LOW) { osc_cursor_pixel = min(ALTO_OSC, osc_cursor_pixel + 2); delay(50); }
                
                // Aceptar posición
                if (digitalRead(PIN_BT_MENU) == LOW) { osc_est = OSC_NORMAL; delay(200); }
                // Cancelar cursor
                if (digitalRead(PIN_BT_BACK) == LOW) { osc_cursor_en = false; osc_est = OSC_MENU; delay(200); }
            }

        }

    }
}
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////




///////////////////////////////*****************************///////////////////////////////
//void convertirPresion_a_Bits () { }




// --- Función Matemática de Regresión Lineal para Calibración ---
void calcularGanancia() {
    r_sumX = 0; r_sumY = 0; r_sumXY = 0; r_sumX2 = 0;
    int n = cal_cantidad_puntos;
    
    for(int i = 0; i < n; i++) {
        float x = cal_teoricos[i]; // El valor que "debería" ser
        float y = cal_medidos[i]; // Presión guardada al aceptar cada punto

        r_sumX += x;
        r_sumY += y;
        r_sumXY += (x * y);
        r_sumX2 += (x * x);
    }
    
    // Cálculos de regresión guardados en variables globales temporales
    float denominador = (n * r_sumX2 - r_sumX * r_sumX);
    if (abs(denominador) < 0.001) {
        r_m = 0.0;
        r_b = 0.0;
        return;
    }

    r_m = (n * r_sumXY - r_sumX * r_sumY) / denominador;
    r_b = (r_sumY - r_m * r_sumX) / n;

    // Cálculo de los parámetros para "forzar" la teórica
    if (abs(r_m) > 0.001) { // Evitar división por cero
        factorGanancia_sinconfirmar = 1.0 / r_m; 
        factorOffset_sinconfirmar = -r_b / r_m; 
    }


}

// --- Lógica Visual de la Pantalla de Calibración ---
void pantallaCalibracion() {
    dibujarHeader("CALIBRACION SENSOR");
    int cx = 35, cy = 40;
    int w = 160, h = 90;
    float maxEscala = getMaxEscala(); // ej: 10.0
    String unidad = getUnidadStr();
    float p_medida = getPresionConvertida(); // Lectura en vivo

    // 1. Gráfico y Fondo
    canvas.fillRect(cx, cy, w, h, TFT_BLACK);
    canvas.drawRect(cx - 1, cy - 1, w + 2, h + 2, C_RECUADRO);
    
    // Escala del Eje Y
    canvas.setTextColor(C_ESCALA);
    canvas.drawString(String(maxEscala, 0), cx - 20, cy, 1);
    canvas.drawString("0", cx - 15, cy + h - 8, 1);

    // 2. Dibujar Grilla (Retícula)
    canvas.setTextColor(C_ESCALA);
    for (int i = 0; i <= h; i += h/10) {   
        // Líneas horizontales punteadas o débiles
        for (int j = 0; j < w; j+=5) {
            canvas.drawPixel(cx + j, cy + i, C_RECUADRO);
            if (i == h && j == w ) canvas.drawString("Medidas", cx + w + 3, cy + h + 6, 1);
        }
    }
    for (int i = 0; i <= w; i += w/10) { 
        // Líneas verticales punteadas o débiles
        for (int j = 0; j < h; j+=5) canvas.drawPixel(cx + i, cy + j, C_RECUADRO);
    }

    // Escala del Eje X 
    for(int i = 0; i <= 10; i++) {
        int tx = cx + (i / 10.0) * w;
        canvas.drawPixel(tx, cy + h + 2, C_ESCALA);
        canvas.drawString(String(i), tx - 3, cy + h + 6, 1);
    }

    // Curva Teórica Ideal (Diagonal 1:1)
    int px_max = cx + w;
    int py_max = cy; 
    canvas.drawLine(cx, cy + h, px_max, py_max, C_ESCALA); 

    // Puntos Rojos Teóricos
    for(int i = 0; i < cal_cantidad_puntos; i++) {
        float porcentaje = cal_teoricos[i] / 10.0;
        float rx = (float)cx + (porcentaje * (float)w);
        float ry = (float)cy + (float)h - (porcentaje * (float)h);
        canvas.fillCircle((int)rx, (int)ry, 2, TFT_RED);
    }

    // Recta de Regresión Calculada (Se dibuja solo al confirmar)
    if (mostrarRegresion) {
        float y_inicial = r_b;
        float y_final = (r_m * maxEscala) + r_b;
        float p_reg_y0 = (float)cy + (float)h - (y_inicial / maxEscala) * (float)h;
        float p_reg_yf = (float)cy + (float)h - (y_final / maxEscala) * (float)h;
        //canvas.drawLine((int)cx, (int)p_reg_y0, (int)cx + (int)w, (int)p_reg_yf, TFT_MAGENTA);
    
        canvas.drawLine((int)cx, constrain((int)p_reg_y0, (int)cy, (int)cy+(int)h), (int)cx + (int)w, constrain((int)p_reg_yf, (int)cy, (int)cy+(int)h), TFT_MAGENTA);
    }
        // Recta de Regresión Calculada (Se dibuja solo al confirmar)
    if (mostrarRegresionCorregida) {
        float y_inicial = (r_b * factorGanancia) + factorOffset; // Debería dar ~0
        float y_final = ((r_m * maxEscala + r_b) * factorGanancia) + factorOffset; // Debería dar ~10
        float p_reg_y0 = (float)cy + (float)h - (y_inicial / maxEscala) * (float)h;
        float p_reg_yf = (float)cy + (float)h - (y_final / maxEscala) * (float)h;
        //canvas.drawLine((int)cx, (int)p_reg_y0, (int)cx + (int)w, (int)p_reg_yf, TFT_MAGENTA);
    
        canvas.drawLine((int)cx, constrain((int)p_reg_y0, (int)cy, (int)cy+(int)h), (int)cx + (int)w, constrain((int)p_reg_yf, (int)cy, (int)cy+(int)h), TFT_MAGENTA);
    }



    // Dibujar los puntos Amarillos Medidos
    for(int i = 0; i < cal_punto_actual && i < cal_cantidad_puntos; i++) {
        float x_med = cal_teoricos[i];
        float px = (float)cx + (x_med / 10.0) * (float)w;

        float presionGuardada = convertirPresionDesdeBar(cal_medidos[i]);
        float py = (float)cy + (float)h - (presionGuardada / maxEscala) * h;
        canvas.fillCircle((int)px, constrain((int)py, (int)cy, (int)cy+(int)h), 2, TFT_YELLOW);
    }

    // 2. Panel Lateral Derecho (Lectura en vivo)
    canvas.setTextColor(TFT_YELLOW);
    canvas.drawString("PRESION", 210 +20 +2, cy, 1);
    canvas.setTextColor(C_TEXTO);
    canvas.drawString(String(p_medida, 2), 210 +20, cy + 15, 4); // Letra grande
    canvas.setTextColor(TFT_YELLOW);
    canvas.drawString(unidad, 210 +20+10+5, cy + 40, 2);

    // 3. Barra de Avance y Estado Inferior
    canvas.setTextColor(C_ESCALA);
    canvas.drawString(modoAscenso ? "Modo: ASCENSO" : "Modo: DESCENSO", 10 + 5, cy + h + 15 + 4, 1);
    int indicePunto = constrain(cal_punto_actual, 0, cal_cantidad_puntos - 1);
    canvas.drawString("Punto " + String(constrain(cal_punto_actual + 1, 1, cal_cantidad_puntos)) + "/" + String(cal_cantidad_puntos) + ": " + String(cal_teoricos[indicePunto]/10.0 * maxEscala, 1) + unidad, 180, cy + h + 15+4, 1);


    // 2. Barra de Avance
    canvas.drawString("Progreso:", 4+ 10+1, 165-1-5, 1);
    canvas.drawRect(80, 165-5, 160, 10, C_RECUADRO);
    float progreso = ((float)constrain(cal_punto_actual, 0, cal_cantidad_puntos) / (float)cal_cantidad_puntos) * 156.0;
    canvas.fillRect(82, 167-5, progreso, 6, TFT_GREEN);


    // 4. Sub-ventanas emergentes (Pop-ups)
    if (estCalib == CAL_CONFIRMAR) {
        canvas.fillRoundRect(50, 50, 220, 80 , 5, TFT_DARKGREY);
        canvas.drawRoundRect(50, 50, 220, 80 , 5, TFT_WHITE);
        
        canvas.setTextColor(TFT_WHITE);
        canvas.drawCentreString("Medicion Tomada", 160, 58, 2);
        
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawCentreString("Presion = " + String(p_capturada_temp, 2) + " " + unidad, 160, 78, 2);
        
        // Botones
        uint16_t colorOk = (cal_seleccion == 0) ? C_ACCENTO : C_FONDO;
        uint16_t colorRepetir = (cal_seleccion == 1) ? TFT_ORANGE : C_FONDO;
        
        canvas.fillRoundRect(65, 100, 80, 20, 3, colorOk);
        canvas.setTextColor(C_TEXTO);
        canvas.drawCentreString("Aceptar", 105, 103 + 2, 1);
        
        canvas.fillRoundRect(175, 100, 80, 20, 3, colorRepetir);
        canvas.drawCentreString("Repetir", 215, 103 + 2, 1);
    }
    
    // NUEVA: Ventana de Resultados de la Regresión
    else if (estCalib == CAL_RESULTADO) {
        canvas.fillRoundRect(30, 30, 260, 160-20, 5, TFT_DARKGREY);
        canvas.drawRoundRect(30, 30, 260, 160-20, 5, TFT_YELLOW);
        
        canvas.setTextColor(TFT_YELLOW);
        canvas.drawCentreString("RESULTADOS DE REGRESION", 160, 35, 2);
        
        // Tabla de datos
        canvas.setTextColor(C_TEXTO);
        int ty = 55;
        canvas.drawString("SumX = " + String(r_sumX, 2), 40, ty, 1);
        canvas.drawString("SumY = " + String(r_sumY, 2), 160, ty, 1);
        canvas.drawString("SumX2= " + String(r_sumX2, 2), 40, ty+15, 1);
        canvas.drawString("SumXY= " + String(r_sumXY, 2), 160, ty+15, 1);
        
        // Valores clave
        canvas.setTextColor(TFT_GREEN);
        canvas.drawString("Ganancia (m): " + String(r_m, 4), 40, ty+35 -10, 2);
        canvas.drawString("Offset (b): " + String(r_b, 4), 40, ty+55 -10, 2);
        
        canvas.setTextColor(C_TEXTO);
        canvas.drawCentreString("valores de la ecuacion Medida", 160, ty+80-5, 1);
        
        // Botón Confirmar
        uint16_t colorConf = (seleccion == 0) ? TFT_GREEN : C_FONDO;
        canvas.fillRoundRect(110, 150-10, 100, 25, 3, colorConf);
        canvas.setTextColor(TFT_BLACK);
        canvas.drawCentreString("Coninuar", 160, 155-10, 2);
    }
        // 6. Submenú Flotante de Configuración
    else if (estCalib == CAL_MENU) {
        int altoMenu = 140; // Antes era 70
        int anchoMenu = 115; // Un poco más ancho para textos largos ("Val Ganancia")

        int mx = cx + ANCHO_OSC - 90;
        int my = cy + 10;
        canvas.fillRoundRect(mx, my, 105+4, 95+2, 3, TFT_DARKGREY);
        canvas.drawRoundRect(mx, my, 105+4, 95+2, 3, C_TEXTO);

        // 3. Variables para el espaciado dinámico de renglones
        int pos_y = 5;       // Posición Y de inicio
        int espaciado = 13;  // Distancia en píxeles entre cada línea


        canvas.setTextColor(C_TEXTO);
        // Índice 2: Línea Verde (Regresión/Calibración) - AGREGADO PERTINENTE
        // Suponiendo que tienes una variable bool llamada mostrarRectaVerde
        //canvas.drawString(osc_menu_sel == 2 ? "> L.Verde: " + String(mostrarRectaVerde ? "Si" : "No") : "  L.Verde: " + String(mostrarRectaVerde ? "Si" : "No"), mx + 5, my + pos_y, 1); pos_y += espaciado;
        
        // === SECCIÓN 2: PARÁMETROS EDITABLES ===
        

        // Índice 5: Activar/Desactivar Offset (Toggle)
        canvas.setTextColor(C_TEXTO);
        canvas.drawString(cal_menu_sel == 0 ? "> Uso Off: " + String(offsetOn ? "ON" : "OFF") : "  Uso Off: " + String(offsetOn ? "ON" : "OFF"), mx + 5, my + pos_y, 1); pos_y += espaciado;

        // Índice 6: Valor de Offset (Editable numéricamente)
        canvas.setTextColor((cal_menu_sel == 1 && editando_cal) ? C_EDIT : C_TEXTO);
        // Usamos factorOffset,2 para mostrar solo 2 decimales
        canvas.drawString(cal_menu_sel == 1 ? "> Val Off: " + String(factorOffset, 2) : "  Val Off: " + String(factorOffset, 2), mx + 5, my + pos_y, 1); pos_y += espaciado;

        // Índice 7: Activar/Desactivar Ganancia (Toggle)
        canvas.setTextColor(C_TEXTO);
        canvas.drawString(cal_menu_sel == 2 ? "> Uso Gan: " + String(ganancia ? "ON" : "OFF") : "  Uso Gan: " + String(ganancia ? "ON" : "OFF"), mx + 5, my + pos_y, 1); pos_y += espaciado;

        // Índice 8: Valor de Ganancia (Editable numéricamente)
        canvas.setTextColor((cal_menu_sel == 3 && editando_cal) ? C_EDIT : C_TEXTO);
        canvas.drawString(cal_menu_sel == 3 ? "> Val Gan: " + String(factorGanancia, 2) : "  Val Gan: " + String(factorGanancia, 2), mx + 5, my + pos_y, 1); pos_y += espaciado;

        canvas.setTextColor(C_TEXTO);
        canvas.drawString(cal_menu_sel == 4 ? "> Regre. Med: " + String(mostrarRegresion ? "ON" : "OFF") : "  Regre. Med: " + String(mostrarRegresion ? "ON" : "OFF"), mx + 5, my + pos_y, 1); pos_y += espaciado;

        canvas.setTextColor((cal_menu_sel == 5 && editando_cal) ? C_EDIT : C_TEXTO);
        canvas.drawString(cal_menu_sel == 5 ? "> Puntos: " + String(cal_cantidad_puntos) : "  Puntos: " + String(cal_cantidad_puntos), mx + 5, my + pos_y, 1); pos_y += espaciado;
        
        // === SECCIÓN 3: SALIDA ===
        
        // Índice 9: Salir
        canvas.setTextColor(C_TEXTO);
        canvas.drawString(cal_menu_sel == 6 ? "> Salir" : "  Salir", mx + 5, my + pos_y, 1);

    }
    else if (estCalib == CAL_SUBMENU) {
        if (abs(r_m) > 0.001){
            int ty = 55;
            canvas.fillRoundRect(50, 50, 220, 120 , 5, TFT_DARKGREY);
            canvas.drawRoundRect(50, 50, 220, 120 , 5, TFT_WHITE);
            
            canvas.setTextColor(TFT_WHITE);
            canvas.drawCentreString("Confirmar valores", 160, 58, 2);
            
            canvas.setTextColor(TFT_GREEN);
            canvas.drawString("Ganancia (m): " + String(factorGanancia_sinconfirmar, 4), 40+50, ty+35 -10, 2);
            canvas.drawString("Offset (b): " + String(factorOffset_sinconfirmar, 4), 40+50, ty+55 -10, 2);
            
            // Botones
            uint16_t colorOk = (cal_seleccion_sub == 0) ? C_ACCENTO : C_FONDO;
            uint16_t colorRepetir = (cal_seleccion_sub == 1) ? TFT_ORANGE : C_FONDO;
            
            canvas.fillRoundRect(65, 100+20, 80, 20, 3, colorOk);
            canvas.setTextColor(C_TEXTO);
            canvas.drawCentreString("Aceptar", 105, 103 + 20, 1);
            
            canvas.fillRoundRect(175, 100+20, 80, 20, 3, colorRepetir);
            canvas.drawCentreString("Cancelar", 215, 103 + 20, 1);
        } else {
            int ty = 55;
            canvas.fillRoundRect(50, 50, 220, 104 , 5, TFT_DARKGREY);
            canvas.drawRoundRect(50, 50, 220, 104 , 5, TFT_WHITE);
            
            canvas.setTextColor(TFT_WHITE);
            canvas.drawCentreString("Valores predefinidos m ~ 0", 160, 58, 2);
            
            canvas.setTextColor(TFT_GREEN);
            canvas.drawString("Ganancia (m): " + String(factorGanancia, 2), 40+50+5, ty+35 -10, 2);
            canvas.drawString("Offset (b): " + String(factorOffset, 2), 40+50+10, ty+55 -10, 2);
            
            // Botones
            uint16_t colorOk = (cal_seleccion == 0) ? C_ACCENTO : C_FONDO;
            uint16_t colorRepetir = (cal_seleccion == 1) ? TFT_ORANGE : C_FONDO;
            
            canvas.fillRoundRect(65+45+5, 100+25, 80, 20, 3, colorOk);
            canvas.setTextColor(C_FONDO);
            canvas.drawCentreString("Aceptar", 150+5, 103 + 27, 1);
        }

    }

}

// --- Función de ayuda para resetear puntos ---
void configurarPuntosTeoricos() {
    float paso = (cal_cantidad_puntos > 1) ? 7.0 / (float)(cal_cantidad_puntos - 1) : 0.0;

    for(int i=0; i<CAL_MAX_PUNTOS; i++) {
        float valor = (i < cal_cantidad_puntos) ? paso * (float)i : 0.0;
        cal_teoricos[i] = modoAscenso ? valor : 7.0 - valor;
    }
}

void obtenervaloresGan_Off() {
    if (abs(r_m) > 0.001) {
        factorGanancia = factorGanancia_sinconfirmar;
        factorOffset = factorOffset_sinconfirmar;
    }
}
///////////////////////////////*****************************///////////////////////////////





//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
void calcularPromedio() {
    if (promedio) {
        // 1. TOMAR MUESTRA CADA 100ms
        unsigned long ahora = millis();

        if (ahora - ultimamuestra >= intervalo) {
            ultimamuestra = ahora;
            
            // 2. LECTURA Y CALIBRACIÓN POR BOTÓN
            valorADC_crudo = analogRead(PIN_BT_SENSOR);
    
            buffer[bufindex] = valorADC_crudo;                              // Guardamos la muestra actual en el buffer circular
            bufindex++;                         

        // Si llegamos al límite de 'muestras' (el elegido en el menú), volvemos a 0
            if (bufindex >= muestras) {
                bufindex = 0;
                buflleno = true;
            }
        }
        // 3. CALCULAR PROMEDIO DEL BUFFER
        // Operador Ternario ? :
        // Funciona como un if de una sola línea. La estructura es:
        // Condición ? Valor_si_es_verdadero : Valor_si_es_falso;
        int totalmuestras = buflleno ? muestras : max(bufindex, 1);

        // max(bufindex, 1) si el bufindex es cero, toma el 1 que es un valor mas grande
        float sumabits = 0;     // Acumulador para la suma de las muestras, empieza en 0 cada vez que calculamos el promedio
        for (int i = 0; i < totalmuestras; i++) 
            sumabits += buffer[i];
        valorADC_crudo_prom = sumabits / totalmuestras;

    } else {
        // 1. TOMAR MUESTRA CADA 100ms
        buflleno = false;
        bufindex = 0;
        unsigned long ahora = millis();

        if (ahora - ultimamuestra >= intervalo) {
            ultimamuestra = ahora;
            
            // 2. LECTURA Y CALIBRACIÓN POR BOTÓN
            valorADC_crudo = analogRead(PIN_BT_SENSOR);
            valorADC_crudo_prom = valorADC_crudo;
    
        }
        // Si no ha pasado el tiempo del intervalo, valorADC_crudo_prom 
        // conserva el valor de la lectura anterior gracias al 'static'
    }



}

void calcularOffset() {
    // Si esta habilitado el offset y presionas el botón (LOW) y no hay falla de lazo del sensor, se calibra el sistema 
    if (offsetOn) {
        if (digitalRead(PIN_BT_OFFSET) == LOW) {
            // Solo calibramos si el sensor está conectado
            if (valorADC_crudo_prom > umbral) { 
                offsetBits = valorADC_crudo_prom - bitmin;               // 7 es el valor teórico para 4mA
                delay(200);                                         // Pequeña demora para evitar rebotes del botón y lecturas erráticas, también da tiempo a que el usuario suelte el botón después de calibrar
            }
        }
 /*
        else if (estCalib == CAL_RESULTADO) {
            // Solo calibramos si el sensor está conectado
            if (valorADC_crudo_prom > umbral) { 
                offsetBits = valorADC_crudo_prom - ;               // 7 es el valor teórico para 4mA
                delay(200);                                         // Pequeña demora para evitar rebotes del botón y lecturas erráticas, también da tiempo a que el usuario suelte el botón después de calibrar
            }

        } */
    } else {
        offsetBits = 0;
    }
    // Aplicamos el offset a la lectura
    valorADC_prom = valorADC_crudo_prom - offsetBits;         // puede surgir un problema que al estar offsetOn y al presionar el boton de OFFSET, se almacena en offsetBits un valor en bits al presionar offset off queda almacenada en offsetBits el valor anterios,


}

void habilitarZonaMuertaValoresNegativos() {
    if (zonaMuerta) {
        if (abs(valorADC_prom - bitmin) > ZONA_MUERTA) {
            corrienteActual = (float)map(valorADC_prom, bitmin, bitmax, 400, 2000) / 100.0;
            presionActual = (corrienteActual - 4.0) * (10.0 / 16.0);    // Cada 0.625 bar es 100 mA, y 4 mA es el punto de inicio (0 bar)
        }
        // presion_s = max(presion_s, 0.0f);            // Evita presiones negativas por ruido, si la presión calculada es menor a 0, la ajusta a 0, la f es para indicar que es un float literal
        else {
            presionActual = 0.0; // Si la zona muerta está activada, forzamos la presión a 0 para evitar fluctuaciones por ruido
            corrienteActual = 4;
        }
    }
    else {
        corrienteActual = (float)map(valorADC_prom, bitmin, bitmax, 400, 2000) / 100.0;
        presionActual = (corrienteActual - 4.0) * (10.0 / 16.0); 
    }


    
    if (!valoresNegativos) {
        if (presionActual < 0) presionActual = 0;
        if (corrienteActual < 4) corrienteActual = 4;

    }


}

void habilitarGanancia() {
    if (ganancia) {
        presionActual = presionActual * factorGanancia + factorOffset;  // sino funciona cambiar borrar factorOfset
    }

}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////






void loop() {
    actualizarColores();


    calcularPromedio();


    calcularOffset();

  
    habilitarZonaMuertaValoresNegativos();


    habilitarGanancia();
    
    if (modoMonitorActivo) {
        // Mandamos los datos separados por comas: presion, corriente, bits
        // Al usar Serial.print y terminar con Serial.println, Python sabe dónde termina la línea
        Serial.print(presionActual);
        Serial.print(",");
        Serial.print(corrienteActual);
        Serial.print(",");
        Serial.println(valorADC_crudo);
        
        // Un pequeño delay para no colgar el puerto USB (muestreo a 10Hz)
        delay(100); 
    }


    switch (estadoActual) {

        case PRINCIPAL: {
            pantallaPrincipal();
            if (digitalRead(PIN_BT_MENU) == LOW) { 
                estadoActual = MENU_RAIZ; 
                seleccion = 0; 
                offsetScroll = 0; 
                delay(250); }
            // if (digitalRead(PIN_BT_OFFSET) == LOW && analogRead(SENSOR_PIN) > umbral) { offsetBits = analogRead(SENSOR_PIN) - bitmin; delay(200); }
            break;
        }

        case MENU_RAIZ: {
            String raiz[] = {"Config. Pantalla", "Config. Datos", "Unidades", "Visual", "Monitor", "Calibracion", "Informacion", "Ayuda"};
            dibujarMenu("MENU PRINCIPAL", raiz, 8);
            if (digitalRead(PIN_BT_UP) == LOW) { seleccion = (seleccion <= 0) ? 7 : seleccion - 1; delay(200); }
            if (digitalRead(PIN_BT_DOWN) == LOW) { seleccion = (seleccion >= 7) ? 0 : seleccion + 1; delay(200); }
            if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = PRINCIPAL; delay(250); }
            if (digitalRead(PIN_BT_MENU) == LOW) {
                if (seleccion == 0) estadoActual = CONF_PANTALLA;
                if (seleccion == 1) estadoActual = CONF_DATOS;
                if (seleccion == 2) estadoActual = UNIDADES;
                if (seleccion == 3) estadoActual = VISUAL;
                if (seleccion == 4) estadoActual = MONITOR;
               // --- BLOQUE DE CALIBRACION ACTUALIZADO ---
                if (seleccion == 5) { 
                    estadoActual = CALIBRACION;
                    estCalib = CAL_MIDIENDO;    // Reset del sub-estado
                    cal_punto_actual = 0;       // Empezar desde el punto 1
                    mostrarRegresion = false;   // Borrar línea magenta previa
                    configurarPuntosTeoricos(); // Cargar los valores (1.0, 2.0, etc.)
                }
                if (seleccion == 6) estadoActual = INFO;
                if (seleccion == 7) estadoActual = AYUDA;
                seleccion = 0; offsetScroll = 0; delay(250);
            }
            break;
        }

        case UNIDADES: {
            String unis[] = {"Bar", "Psi", "Pa", "kgf/cm²", "m.c.a", "Atm"};
            dibujarMenu("UNIDADES", unis, 6);
            if (digitalRead(PIN_BT_UP) == LOW) { seleccion = (seleccion <= 0) ? 5 : seleccion - 1; delay(200); }
            if (digitalRead(PIN_BT_DOWN) == LOW) { seleccion = (seleccion >= 5) ? 0 : seleccion + 1; delay(200); }
            
            int yPosTarget = 42 + ((unidadActual - offsetScroll) * 30);
            if (unidadActual >= offsetScroll && unidadActual < offsetScroll + 4) {
                canvas.fillCircle(250, yPosTarget + 13, 5, TFT_GREEN);
            }

            if (digitalRead(PIN_BT_MENU) == LOW) { unidadActual = (UnidadMedida)seleccion; delay(200); }
            if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = MENU_RAIZ; seleccion = 2; offsetScroll = 0; delay(250); }
            break;
        }

        case CONF_PANTALLA: {
            String pant[] = {
                "Modo: " + String(modoOscuro?"Oscuro":"Claro"), 
                "Brillo: " + String(map(brillo, 50, 255, 1, 5)), 
                "Retroilum: " + String(retroIlum?"ON":"OFF"), 
                "Back"
            };
            dibujarMenu("CONFIG. PANTALLA", pant, 4);
            if (digitalRead(PIN_BT_UP) == LOW) { seleccion = (seleccion <= 0) ? 3 : seleccion - 1; delay(200); }
            if (digitalRead(PIN_BT_DOWN) == LOW) { seleccion = (seleccion >= 3) ? 0 : seleccion + 1; delay(200); }
            
            if (digitalRead(PIN_BT_MENU) == LOW) {
                if (seleccion == 0) modoOscuro = !modoOscuro;
                if (seleccion == 1) { brillo = (brillo + 41 > 256) ? 50 : brillo + 41; analogWrite(PIN_BACKLIGHT, brillo); }
                if (seleccion == 2) {retroIlum = !retroIlum; (retroIlum) ? (ledRGB.setPixelColor(0, ledRGB.Color(255, 0, 0)), ledRGB.show()) : (ledRGB.setPixelColor(0, ledRGB.Color(0, 0, 0)), ledRGB.show());}
                if (seleccion == 3) { estadoActual = MENU_RAIZ; seleccion = 0; offsetScroll = 0; }
                delay(200);
            }
            if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = MENU_RAIZ; seleccion = 0; offsetScroll = 0; delay(250); }
            break;
        }

        case CONF_DATOS: {
            String datos[] = {
                "BitMin: " + String(bitmin), 
                "BitMax: " + String(bitmax), 
                "Muestras: " + String(muestras), 
                "Intervalo: " + String(intervalo) + " ms", 
                "Umbral: " + String(umbral),
                "Resistencia: " + String(resistencia) + " Ohm", 
                "Zona Muerta: " + String(zonaMuerta?"ON":"OFF"), 
                "Uso Offset: " + String(offsetOn?"ON":"OFF"), 
                "Promedio: " + String(promedio?"ON":"OFF"),
                "Valores Negativos: " + String(valoresNegativos?"ON":"OFF"), 
                "Uso Ganancia: " + String(ganancia?"ON":"OFF"),
                "Factor Ganancia: " + String(factorGanancia, 2),
                "Factor Offset " + String(factorOffset, 2),
                "Back"
            };
            dibujarMenu("CONFIG. DATOS", datos, 14);

            if (!editando) {
                // Navegación Normal
                if (digitalRead(PIN_BT_UP) == LOW) { seleccion = (seleccion <= 0) ? 13 : seleccion - 1; delay(200); }
                if (digitalRead(PIN_BT_DOWN) == LOW) { seleccion = (seleccion >= 13) ? 0 : seleccion + 1; delay(200); }
                if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = MENU_RAIZ; seleccion = 1; offsetScroll = 0; delay(250); }
                
                if (digitalRead(PIN_BT_MENU) == LOW) {
                    if (seleccion <= 5) editando = true; // Entra a modo edición para números
                    else if (seleccion == 6) zonaMuerta = !zonaMuerta; // Toggles directos
                    else if (seleccion == 7) offsetOn = !offsetOn;
                    else if (seleccion == 8) promedio = !promedio;
                    else if (seleccion == 9) valoresNegativos = !valoresNegativos;
                    else if (seleccion == 10) ganancia = !ganancia;
                    else if (seleccion >= 11 && seleccion <= 12) editando = true; // este && hace :  
                    else if (seleccion == 13) { estadoActual = MENU_RAIZ; seleccion = 1; offsetScroll = 0; }
                    delay(250);
                }
            } else {
                // Modo Edición (Cambiar valores de 1 en 1 o 10 en 10)
                if (digitalRead(PIN_BT_UP) == LOW) {
                    if (tPresionado == 0) tPresionado = millis();
                    int inc = (millis() - tPresionado > 600) ? 10 : 1; // Si pasa 600ms, sube de a 10
                    modificarValorConfig(seleccion, inc);
                    delay((inc == 10) ? 80 : 200); // Rápido si mantiene, normal si toca una vez
                } 
                else if (digitalRead(PIN_BT_DOWN) == LOW) {
                    if (tPresionado == 0) tPresionado = millis();
                    int inc = (millis() - tPresionado > 600) ? 10 : 1;
                    modificarValorConfig(seleccion, -inc);
                    delay((inc == 10) ? 80 : 200);
                } 
                else {
                    tPresionado = 0; // Se soltó el botón
                }
                
                if (digitalRead(PIN_BT_MENU) == LOW || digitalRead(PIN_BT_BACK) == LOW) {
                    editando = false; tPresionado = 0; delay(250);
                }
            }
            break;
        }

        case VISUAL: {
            String vis[] = {"Analogico", "Digital", "Parametros", "Graficadora"};
            dibujarMenu("VISUALIZACION", vis, 4);
            if (digitalRead(PIN_BT_UP) == LOW) { seleccion = (seleccion <= 0) ? 3 : seleccion - 1; delay(200); }
            if (digitalRead(PIN_BT_DOWN) == LOW) { seleccion = (seleccion >= 3) ? 0 : seleccion + 1; delay(200); }
            
            // Dibuja un puntito verde en la opción actualmente activa
            int yPosTarget = 42 + (modoVista * 30);
            canvas.fillCircle(250, yPosTarget + 13, 5, TFT_GREEN);
            
            if (digitalRead(PIN_BT_MENU) == LOW) { modoVista = (ModoVisual)seleccion; delay(200); }
            if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = MENU_RAIZ; seleccion = 3; delay(250); }
            break;
        }

        case MONITOR: {
            dibujarHeader("MONITOR SERIAL");
            
            // --- Lógica de animación del Switch ---
            // Usamos 'static' para que recuerde su posición entre ciclos del loop
            static int posKnobX = 35; // Posición X inicial del botón (OFF)
            int targetX = modoMonitorActivo ? 65 : 35; // Hacia dónde debe ir
            
            // Interpolar la posición (suavizado del movimiento)
            if (posKnobX < targetX) posKnobX += 4; // Velocidad de ida
            if (posKnobX > targetX) posKnobX -= 4; // Velocidad de vuelta

            // Determinar color de fondo del switch
            uint16_t colorFondoSwitch = modoMonitorActivo ? TFT_GREEN : TFT_DARKGREY;
            
            // 1. Dibujar fondo del switch (cápsula redondeada)
            canvas.fillRoundRect(20, 60, 60, 30, 15, colorFondoSwitch);
            
            // 2. Dibujar el botón deslizante (Knob blanco)
            // Agregamos un borde fino gris oscuro para darle volumen
            canvas.fillCircle(posKnobX, 75, 12, TFT_WHITE);
            canvas.drawCircle(posKnobX, 75, 12, C_RECUADRO);

            // --- Textos ---
            canvas.setTextColor(C_TEXTO);
            if (modoMonitorActivo) {
                canvas.drawString("Estado: ON", 90, 67, 2);
                canvas.setTextColor(TFT_GREEN);
                canvas.drawString("Transmitiendo por USB...", 20, 100, 2);
            } else {
                canvas.drawString("Estado: OFF", 90, 67, 2);
                canvas.setTextColor(C_ESCALA);
                canvas.drawString("Puerto USB en espera", 20, 100, 2);
            }
            
            canvas.setTextColor(C_ACCENTO);
            canvas.drawString("Presione MENU para cambiar", 20, 130, 2);

            // --- Controles de Botones ---
            // Si presionás el botón menú, invertís el estado (ON -> OFF / OFF -> ON)
            if (digitalRead(PIN_BT_MENU) == LOW) {
                modoMonitorActivo = !modoMonitorActivo;
                delay(200); // Anti-rebote aumentado levemente para más seguridad
            }

            if (digitalRead(PIN_BT_BACK) == LOW) { 
                estadoActual = MENU_RAIZ; 
                seleccion = 4; 
                delay(200); 
            }
            break;
        }
       
       
        case CALIBRACION: {
            pantallaCalibracion(); // Dibuja todo

            // --- LÓGICA DE BOTONES POR ESTADO ---
            switch(estCalib) {
                
                case CAL_MIDIENDO:{
                    // 1. Cambiar Modo (UP)
                    if (digitalRead(PIN_BT_DOWN) == LOW) {
                        modoAscenso = !modoAscenso;
                        configurarPuntosTeoricos();
                        delay(200);
                    }
                    // 1. Cambiar Modo (DOWN)
                    if (digitalRead(PIN_BT_UP) == LOW) {
                        estCalib = CAL_MENU;
                        editando_cal = false;
                        delay(200);
                    }

                    // 2. Capturar (MENU)
                    if (digitalRead(PIN_BT_MENU) == LOW) {
                        if (cal_punto_actual < cal_cantidad_puntos){
                            p_capturada_temp = getPresionConvertida(); // "Congela" el valor para el popup
                            p_capturada_temp_bar = presionActual;
                            estCalib = CAL_CONFIRMAR;
                            cal_seleccion = 0; // Cursor en "Aceptar"
                            delay(300);
                        }
                        else {

                            mostrarRegresionCorregida = true; // Activa la línea magenta
                            mostrarRegresion = true; // Activa la línea magenta
                            calcularGanancia();
                            pantallaCalibracion(); 
                            estCalib = CAL_RESULTADO;
                            cal_seleccion = 0; // esto hace que el cuadro este en aceptar primero
                            delay(300); 
                        }
                    }
                    // 3. Salir al Menú (BACK)
                    if (digitalRead(PIN_BT_BACK) == LOW) {
                        estadoActual = MENU_RAIZ;
                        seleccion = 5;
                        delay(300);
                    }
                    break;
                }

                case CAL_CONFIRMAR: {
                    // Navegar entre botones del Popup (UP/DOWN)
                    if (digitalRead(PIN_BT_UP) == LOW || digitalRead(PIN_BT_DOWN) == LOW) {
                        cal_seleccion = !cal_seleccion; 
                        delay(300);

                    }
                    // Seleccionar opción (MENU)
                    if (digitalRead(PIN_BT_MENU) == LOW) {
                        if (cal_seleccion == 0) { // ACEPTAR
                            cal_medidos[cal_punto_actual] = p_capturada_temp_bar;
                            cal_punto_actual++;
                            if (cal_punto_actual >= cal_cantidad_puntos) {
                                calcularGanancia();
                                mostrarRegresionCorregida = true;
                                mostrarRegresion = true;
                                estCalib = CAL_RESULTADO;
                                cal_seleccion = 0; 
                                delay(300);
                            } else {
                                estCalib = CAL_MIDIENDO;
                                delay(300);
                            }
                        } else { // REPETIR
                            estCalib = CAL_MIDIENDO;
                            delay(300);
                        }
                        
                    }
                    if (digitalRead(PIN_BT_BACK) == LOW) {
/*                         estCalib = CAL_MIDIENDO;
                        cal_seleccion = 0; 
                        delay(300); */
                        estadoActual = MENU_RAIZ;
                        seleccion = 5;
                        delay(300);
                    }
                    break;
                }
                
                case CAL_RESULTADO: {
                    if (digitalRead(PIN_BT_MENU) == LOW) {
                        //calcularGanancia();
                        // 2. FORZAMOS el dibujo del gráfico con el 8vo punto visible
                        // Ponemos estCalib en un estado temporal o simplemente llamamos a la función
                        //pantallaCalibracion(); 

                        /* factorGanancia = r_m;
                        // Convertir el factorOffset (Bar) a valor de ADC (Bits)
                        factorOffset = r_b; */
                        //mostrarRegresionCorregida = true; // Activa la línea magenta
                        //mostrarRegresion = true; // Activa la línea magenta
                        estCalib = CAL_SUBMENU; // Vuelve a la pantalla base para ver el gráfico
                        cal_punto_actual = 0;    // Reset para volver a iniciar con el vetor de datos
                        delay(300);
                    }
                    if (digitalRead(PIN_BT_BACK) == LOW) {
                        estCalib = CAL_MIDIENDO;
                        cal_punto_actual = 0;
                        delay(300);
                    }
                    break;
                }
                
                case CAL_MENU:{
                    if (!editando_cal) {
                        // Navegar normal del pequeño submenú
                        if (digitalRead(PIN_BT_UP) == LOW) { cal_menu_sel = max(0, cal_menu_sel - 1); delay(200); }
                        if (digitalRead(PIN_BT_DOWN) == LOW) { cal_menu_sel = min(6, cal_menu_sel + 1); delay(200); }
                        
                        if (digitalRead(PIN_BT_MENU) == LOW) {
                            if (cal_menu_sel == 0) offsetOn = !offsetOn; 
                            if (cal_menu_sel == 1) editando_cal = true; 
                            if (cal_menu_sel == 2) ganancia = !ganancia; 
                            if (cal_menu_sel == 3) editando_cal = true; 
                            if (cal_menu_sel == 4) mostrarRegresion = !mostrarRegresion; 
                            if (cal_menu_sel == 5) editando_cal = true;
                            if (cal_menu_sel == 6) estCalib = CAL_MIDIENDO;
                            delay(200);
                        }
                        if (digitalRead(PIN_BT_BACK) == LOW) { estCalib = CAL_MIDIENDO; editando_cal = false; delay(200); }

                    }else if (editando_cal) {
                        // Modo Edición (Cambiar valores de 1 en 1 o 10 en 10)
                        if (digitalRead(PIN_BT_UP) == LOW) {
                            if (tPresionado_cal == 0) tPresionado_cal = millis();
                            int inc = (millis() - tPresionado_cal > 600) ? 10 : 1; // Si pasa 600ms, sube de a 10
                            modificarValorMenuCal(cal_menu_sel, inc);
                            delay((inc == 10) ? 80 : 200); // Rápido si mantiene, normal si toca una vez
                        } 
                        else if (digitalRead(PIN_BT_DOWN) == LOW) {
                            if (tPresionado_cal == 0) tPresionado_cal = millis();
                            int inc = (millis() - tPresionado_cal > 600) ? 10 : 1;
                            modificarValorMenuCal(cal_menu_sel, -inc);
                            delay((inc == 10) ? 80 : 200);
                        } 
                        else {
                            tPresionado_cal = 0; // Se soltó el botón
                        }
                        
                        if (digitalRead(PIN_BT_MENU) == LOW || digitalRead(PIN_BT_BACK) == LOW) {
                            editando_cal = false; tPresionado_cal = 0; delay(200);
                        }
                    }
                    else {
                        if (digitalRead(PIN_BT_BACK) == LOW) { estCalib = CAL_MIDIENDO; editando_cal = false; delay(200); }
                    }
                    break;
                }
                
                case CAL_SUBMENU:{
                    if (digitalRead(PIN_BT_BACK) == LOW) { 
                        estCalib = CAL_MIDIENDO; 
                        cal_punto_actual = 0;
                        delay(200); }

                    if (r_m >= 0.001) {  // Si la pendiende de la recta medida no esta proxima a cero, hay dos estados, aceptar los valores y actualizar o carcelar esos valores.
                        // Navegar entre botones del Popup (UP/DOWN)
                        if (digitalRead(PIN_BT_UP) == LOW || digitalRead(PIN_BT_DOWN) == LOW) {
                            cal_seleccion_sub = !cal_seleccion_sub; 
                            delay(300);
                        }

                        // Seleccionar aceptar valor de m y b.
                        if (digitalRead(PIN_BT_MENU) == LOW) {
                            if (cal_seleccion_sub == 0) { // ACEPTAR
                                obtenervaloresGan_Off();   // Al llamar esta funcion actualiza los valores sin confirmar.
                                estCalib = CAL_MIDIENDO;
                                delay(300);
                            } else { // Cancelar el valor de m y b. quedarse con los ya configurados
                                estCalib = CAL_MIDIENDO;
                                delay(300);
                            }
                        }

                    }
                    else  {
                        if (digitalRead(PIN_BT_MENU) == LOW ) {
                            estCalib = CAL_MIDIENDO; // Vuelve a la pantalla base para ver el gráfico
                            delay(300); 
                        }
                    } 

                    break;   
                }
                
                break;  
            }
            break; 
        }
        
        case INFO: {
            dibujarHeader("INFORMACION");
            canvas.setTextColor(C_TEXTO);

            canvas.drawString("Autores: Estudiantes UNNE", 20, 50, 2);
            canvas.drawString("Pereson, Brian Nicolas", 20, 80, 2);
            canvas.drawString("Guillen, Facundo Nicolas", 20, 110, 2);
            canvas.drawString("Version: 1.0 S3-LCD", 20, 140, 2);
            if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = MENU_RAIZ; seleccion = 6; delay(250); }
            break;
        }
        
        case AYUDA: {
            dibujarHeader("AYUDA");
            canvas.setTextColor(C_TEXTO);
            
            canvas.drawString("Instrucciones de uso", 20, 50, 2);
            canvas.drawString("Presione MENU para entrar", 20, 80, 2);
            canvas.drawString("en modo edición", 20, 110, 2);
            if (digitalRead(PIN_BT_BACK) == LOW) { estadoActual = MENU_RAIZ; seleccion = 7; delay(250); }
            break;
        }
    }

    canvas.pushSprite(0, 0);
}
    
