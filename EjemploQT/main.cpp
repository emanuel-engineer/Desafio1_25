#include <iostream>
#include <fstream>
#include <QImage>
#include <QCoreApplication>

using namespace std;

// ====================== RUTAS DE ARCHIVOS ======================
const char* RUTA_IO = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/P1.bmp";
const char* RUTA_IM = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/I_M.bmp";
const char* RUTA_M = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/M.bmp";
const char* RUTA_M1 = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/M1.txt";
const char* RUTA_M2 = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/M2.txt";
const char* RUTA_SALIDA = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/build/salida.bmp";

// ====================== DECLARACIÓN DE FUNCIONES ======================
unsigned char* cargar_imagen(const char* ruta, int& ancho, int& alto);
bool guardar_imagen(unsigned char* datos, int ancho, int alto, const char* ruta);
void aplicar_xor(unsigned char* imagen1, unsigned char* imagen2, int tamaño);
void rotar_bits(unsigned char* imagen, int tamaño, int bits);
bool verificar_enmascaramiento(unsigned char* io, unsigned char* m, const char* ruta_masking, int& offset);
void procesar_imagen(unsigned char* io, unsigned char* im, unsigned char* m, int ancho, int alto);

// ====================== MAIN ======================
int main() {
    int ancho, alto;
    unsigned char* io = cargar_imagen(RUTA_IO, ancho, alto);
    unsigned char* im = cargar_imagen(RUTA_IM, ancho, alto);
    unsigned char* m = cargar_imagen(RUTA_M, ancho, alto);

    if (!io || !im || !m) {
        cerr << "Error al cargar imágenes" << endl;
        return -1;
    }

    procesar_imagen(io, im, m, ancho, alto);
    guardar_imagen(io, ancho, alto, RUTA_SALIDA);

    delete[] io;
    delete[] im;
    delete[] m;
    cout << "Proceso completado. Imagen guardada en " << RUTA_SALIDA << endl;
    return 0;
}
// ====================== IMPLEMENTACIÓN DE FUNCIONES ======================
unsigned char* cargar_imagen(const char* ruta, int& ancho, int& alto) {
    QImage imagen(ruta);
    if (imagen.isNull()) {
        cerr << "Error al cargar: " << ruta << endl;
        return nullptr;
    }

    imagen = imagen.convertToFormat(QImage::Format_RGB888);
    ancho = imagen.width();
    alto = imagen.height();

    unsigned char* datos = new unsigned char[ancho * alto * 3];
    for (int y = 0; y < alto; ++y) {
        memcpy(datos + y * ancho * 3, imagen.scanLine(y), ancho * 3);
    }

    return datos;
}

bool guardar_imagen(unsigned char* datos, int ancho, int alto, const char* ruta) {
    QImage imagen(ancho, alto, QImage::Format_RGB888);
    for (int y = 0; y < alto; ++y) {
        memcpy(imagen.scanLine(y), datos + y * ancho * 3, ancho * 3);
    }
    return imagen.save(ruta, "BMP");
}

void aplicar_xor(unsigned char* imagen1, unsigned char* imagen2, int tamaño) {
    for (int i = 0; i < tamaño; ++i) {
        imagen1[i] ^= imagen2[i];
    }
}

void rotar_bits(unsigned char* imagen, int tamaño, int bits) {
    for (int i = 0; i < tamaño; ++i) {
        imagen[i] = (imagen[i] >> bits) | (imagen[i] << (8 - bits));
    }
}

bool verificar_enmascaramiento(unsigned char* io, unsigned char* m, const char* ruta_masking, int& offset) {
    ifstream archivo(ruta_masking);
    if (!archivo.is_open()) {
        cerr << "Error al abrir " << ruta_masking << endl;
        return false;
    }

    archivo >> offset;

    QImage mascara(RUTA_M);
    int m_width = mascara.width();
    int m_height = mascara.height();
    int m_size = m_width * m_height * 3;

    int io_size = ancho * alto * 3; // Asume que 'ancho' y 'alto' son globales o calculados

    int r, g, b;
    int k = 0;
    while (archivo >> r >> g >> b && k < m_size) {
        int pos = (offset + k) % io_size;
        if ((io[pos] + m[k]) != r ||
            (io[pos+1] + m[k+1]) != g ||
            (io[pos+2] + m[k+2]) != b) {
            archivo.close();
            return false;
        }
        k += 3;
    }
    archivo.close();
    return true;
}

void liberar_datos(unsigned int** datos, int num_archivos) {
    for (int i = 0; i < num_archivos; ++i) {
        if (datos[i]) {
            delete[] datos[i];
        }
    }
    delete[] datos;
}

void procesar_imagen(unsigned char* io, unsigned char* im, unsigned char* m, int ancho, int alto) {
    // 1. Aplicar XOR inverso (para revertir la transformación)
    aplicar_xor(io, im, ancho * alto * 3);

    // 2. Rotar bits (ejemplo: 3 bits a la izquierda para revertir)
    rotar_bits(io, ancho * alto * 3, 3);

    // 3. Aplicar XOR inverso nuevamente si es necesario
    aplicar_xor(io, im, ancho * alto * 3);

    // Nota: El orden y parámetros exactos dependen de cómo se transformó originalmente la imagen
}
