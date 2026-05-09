/* Entiendo lo que estás haciendo, Brian. Estás trabajando con el formato RGB565 (16 bits), que es el estándar para la librería TFT_eSPI que usas en tu ESP32-S3.

En este formato, los 16 bits se reparten así: 5 bits para el Rojo, 6 bits para el Verde y 5 bits para el Azul. Lo que vos hacés sumando valores hexadecimales es manipular esos bits directamente.

Aquí tenés una lista de 100 colores organizados por familias, listos para copiar en tu código de Arduino:

Colores Básicos y Primarios
C++ */

#define BLACK       0x0000
#define WHITE       0xFFFF
#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define YELLOW      0xFFE0
#define CYAN        0x07FF
#define MAGENTA     0xF81F
/* Escala de Grises (Útiles para fondos y botones)
C++ */

#define GRAY_1      0x18C3
#define GRAY_2      0x3186
#define GRAY_3      0x4A49
#define GRAY_4      0x632C
#define GRAY_5      0x7BEF
#define GRAY_6      0x94B2
#define GRAY_7      0xAD75
#define GRAY_8      0xC638
#define GRAY_9      0xDEFB
#define DARK_GRAY   0x4208
#define LIGHT_GRAY  0xD69A
/* Rojos y Naranjas (Alertas y Sensores)
C++ */

#define MAROON      0x7800
#define CRIMSON     0xD800
#define DARK_RED    0x8000
#define FIREBRICK   0xB104
#define ORANGE      0xFD20
#define DARK_ORANGE 0xFC60
#define TOMATO      0xFB08
#define GOLD        0xFEA0
#define CORAL       0xFBEA
#define SALMON      0xFC0E
/* Verdes (Estado OK / Sistemas operativos)
C++
 */
#define DARK_GREEN  0x03E0
#define FOREST      0x2444
#define LIME        0x07E0
#define OLIVE       0x7BE0
#define PALE_GREEN  0x9FD3
#define SPRING      0x07EF
#define SEAGREEN    0x2C4A
#define CHARTREUSE  0x7FE0
#define GREEN_YEL   0xAFE5
/* Azules y Cyans (Presión / Agua)
C++ */

#define NAVY        0x000F
#define DARK_BLUE   0x0011
#define MIDNIGHT    0x18CE
#define ROYAL_BLUE  0x435C
#define SKY_BLUE    0x867D
#define DEEP_SKY    0x05FF
#define STEEL_BLUE  0x4416
#define AQUA        0x07FF
#define TEAL        0x0410
#define CADET_BLUE  0x5CF4
/* Púrpuras y Rosas
C++ */

#define PURPLE      0x780F
#define INDIGO      0x4810
#define VIOLET      0x9199
#define PLUM        0xDD1B
#define ORCHID      0xDB9A
#define DEEP_PINK   0xF8B2
#define HOT_PINK    0xFB56
#define LAVENDER    0xE73F
/* Marrones y Tierra (Establecimiento Don Miti)
C++ */

#define BROWN       0xA145
#define SADDLE      0x8A22
#define SIENNA      0xA285
#define CHOCOLATE   0xD343
#define PERU        0xCC27
#define SANDY_BROWN 0xF52C
#define TAN         0xD5B1
#define BEIGE       0xF7BB
/* 50 Tonos Adicionales (Mezclas Técnicas)
C++ */

#define DARK_CYAN    0x03EF
#define DARK_MAGENTA 0x800F
#define DARK_ORCHID  0x9917
#define DARK_VIOLET  0x901A
#define FUCHSIA      0xF81F
#define IVORY        0xFFFE
#define KHAKI        0xF731
#define LINEN        0xF77E
#define MINT_CREAM   0xF7FF
#define MISTY_ROSE   0xFF3C
#define MOCCASIN     0xFF36
#define OLD_LACE     0xFFBC
#define PAPAYA_WHIP  0xFF7A
#define PEACH_PUFF   0xFED7
#define PINK         0xFE19
#define POWDER_BLUE  0xB71C
#define ROSY_BROWN   0xBC71
#define SILVER       0xC618
#define SNOW         0xFFDF
#define THISTLE      0xDDFB
#define WHEAT        0xF6F6
#define WHITE_SMOKE  0xF7BE
#define YELLOW_GREEN 0x9E66
#define DARK_KHAKI   0xBDAD
#define DARK_SALMON  0xECAF
#define LIGHT_CORAL  0xF410
#define LIGHT_SALMON 0xFD0F
#define LIGHT_SEA    0x2595
#define LIGHT_SKY    0x867F
#define LIGHT_SLATE  0x7453
#define LIGHT_STEEL  0xB61B
#define LIGHT_YELLOW 0xFFFC
#define MED_AQUA     0x6675
#define MED_BLUE     0x0019
#define MED_ORCHID   0xBA18
#define MED_PURPLE   0x939B
#define MED_SEA      0x3D8E
#define MED_SLATE    0x7B5D
#define MED_SPRING   0x07D3
#define MED_TURQ     0x4E99
#define MED_VIOLET   0xC713
#define SLATE_BLUE   0x6AD9
#define SLATE_GRAY   0x7412
#define TURQUOISE    0x471A
#define LIGHT_PINK   0xFDB8
#define DARK_SLATE   0x2A69
#define DARK_TURQ    0x067A
#define GOLDEN_ROD   0xDD24
#define LAWN_GREEN   0x7FE0
#define INDIAN_RED   0xCAEB
/* Un truco de Ingeniería para tu código
Como estás sumando valores, si alguna vez necesitás generar un color personalizado a partir de valores RGB (0-255) normales, podés usar esta macro que hace el desplazamiento de bits automáticamente para el formato de 16 bits:

C++ */

/* #define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

// Ejemplo de uso:
uint16_t miColor = RGB565(128, 0, 16); // Esto te daría un Bordo similar al tuyo */