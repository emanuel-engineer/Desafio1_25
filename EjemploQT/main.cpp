#include <iostream>
#include <fstream>
#include <QImage>
#include <QCoreApplication>

using namespace std;

// ====================== RUTAS DE ARCHIVOS ======================
const char* RUTA_IO = "";       // Imagen original transformada
const char* RUTA_IM = "";        // Imagen aleatoria para XOR
const char* RUTA_M = "";          // Máscara para enmascaramiento
const char* RUTA_M1 = "";        // Archivo de enmascaramiento 1
const char* RUTA_M2 = "";        // Archivo de enmascaramiento 2
const char* RUTA_SALIDA = "";// Imagen reconstruida

// ====================== DECLARACIÓN DE FUNCIONES ======================
unsigned char* cargar_imagen(const char* ruta, int& ancho, int& alto);
bool guardar_imagen(unsigned char* datos, int ancho, int alto, const char* ruta);
void aplicar_xor(unsigned char* imagen1, unsigned char* imagen2, int tamaño);
void rotar_bits(unsigned char* imagen, int tamaño, int bits);
unsigned int** cargar_archivos_enmascaramiento(const char** rutas, int num_archivos, int* semillas, int* num_pixeles);
void liberar_datos(unsigned int** datos, int num_archivos);
void procesar_imagen(unsigned char* io, unsigned char* im, unsigned char* m, int ancho, int alto);

