#include <fstream>
#include <iostream>
#include <QCoreApplication>
#include <QImage>

using namespace std;

// Prototipos de funciones
unsigned char* cargarImagen(QString archivo, int &ancho, int &alto);
bool guardarImagen(unsigned char* pixeles, int ancho, int alto, QString archivo);
unsigned char rotarDerecha3bits(unsigned char valor);
unsigned char rotarIzquierda3bits(unsigned char valor);
bool cargarDatosEnmascaramiento(const char* archivo, int &semilla, unsigned int* &datos, int &cantidad);

int main()
{
    // Archivos de entrada
    QString archivoOriginal = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/I_O.bmp";
    QString archivoAleatoria = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/I_M.bmp";
    QString archivoMascara = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/resources/M1.txt";

    // Archivos de salida para cada paso
    QString archivoPaso1 = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/build/P1_XOR.bmp";
    QString archivoPaso2 = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/build/P2_Rotacion.bmp";
    QString archivoPaso3 = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/build/P3_XOR_Final.bmp";
    QString archivoReconstruida = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/build/I_R_Reconstruida.bmp";
    QString archivoPaso4 = "/home/demuusz/UDEA/Desafio1_25/EjemploQT/build/P4_Enmascaramiento.bmp";

    // Cargar dimensiones de las imágenes
    int ancho = 0, alto = 0;
    int ancho2 = 0, alto2 = 0;
    int ancho3 = 0, alto3 = 0;

    cout << "Cargando imagenes..." << endl;
    unsigned char *original = cargarImagen(archivoOriginal, ancho, alto);
    unsigned char *aleatoria = cargarImagen(archivoAleatoria, ancho2, alto2);
    unsigned char *reconstruido = cargarImagen(archivoReconstruida, ancho3 , alto3);


    if(!original || !aleatoria || ancho != ancho2 || alto != alto2) {
        cout << "Error: Las imagenes no coinciden en dimensiones o no se pudieron cargar" << endl;
        return 1;
    }

    int totalPixeles = ancho * alto * 3;

    // ========== PROCESO DE TRANSFORMACIÓN ==========
    cout << "\nAplicando transformaciones..." << endl;

    // Paso 1: XOR entre I_O e I_M
    unsigned char* paso1 = new unsigned char[totalPixeles];
    for(int i = 0; i < totalPixeles; i++) {
        paso1[i] = original[i] ^ aleatoria[i];
    }
    guardarImagen(paso1, ancho, alto, archivoPaso1);
    cout << "Paso 1 completado: XOR aplicado (guardado en " << archivoPaso1.toStdString() << ")" << endl;

    // Paso 2: Rotación 3 bits a la derecha
    unsigned char* paso2 = new unsigned char[totalPixeles];
    for(int i = 0; i < totalPixeles; i++) {
        paso2[i] = rotarDerecha3bits(paso1[i]);
    }
    guardarImagen(paso2, ancho, alto, archivoPaso2);
    cout << "Paso 2 completado: Rotacion 3 bits derecha (guardado en " << archivoPaso2.toStdString() << ")" << endl;

    // Paso 3: XOR entre resultado y I_M
    unsigned char* paso3 = new unsigned char[totalPixeles];
    for(int i = 0; i < totalPixeles; i++) {
        paso3[i] = paso2[i] ^ aleatoria[i];
    }
    guardarImagen(paso3, ancho, alto, archivoPaso3);
    cout << "Paso 3 completado: XOR final (guardado en " << archivoPaso3.toStdString() << ")" << endl;

    // ========== PROCESO DE RECONSTRUCCIÓN ==========
    cout << "\nIniciando reconstruccion usando M1.txt..." << endl;

    // Cargar datos de enmascaramiento
    int semilla = 0;
    unsigned int* datosMascara = nullptr;
    int cantidadPixelesMascara = 0;

    if(!cargarDatosEnmascaramiento(archivoMascara.toStdString().c_str(), semilla, datosMascara, cantidadPixelesMascara)) {
        cout << "Error al cargar el archivo de enmascaramiento" << endl;
        return 1;
    }
    cout << "Datos de enmascaramiento cargados. Semilla: " << semilla
         << ", Pixeles: " << cantidadPixelesMascara << endl;

    // Paso 1 Reconstrucción: XOR inverso (Paso 3 original)
    unsigned char* r_paso1 = new unsigned char[totalPixeles];
    for(int i = 0; i < totalPixeles; i++) {
        r_paso1[i] = paso3[i] ^ aleatoria[i];
    }

    // Paso 2 Reconstrucción: Rotación inversa (izquierda)
    unsigned char* r_paso2 = new unsigned char[totalPixeles];
    for(int i = 0; i < totalPixeles; i++) {
        r_paso2[i] = rotarIzquierda3bits(r_paso1[i]);
    }
    guardarImagen(r_paso2, ancho, alto, archivoPaso4);
    cout << "Pre-reconstruccion completada (guardado en " << archivoPaso4.toStdString() << ")" << endl;

    // Paso 3 Reconstrucción: Aplicar enmascaramiento inverso
    for(int i = 0; i < cantidadPixelesMascara * 3; i += 3) {
        int posicion = (semilla + i) % totalPixeles;
        r_paso2[posicion] = datosMascara[i] - r_paso2[posicion];
        r_paso2[posicion+1] = datosMascara[i+1] - r_paso2[posicion+1];
        r_paso2[posicion+2] = datosMascara[i+2] - r_paso2[posicion+2];
    }

    // Paso 4 Reconstrucción: XOR final con I_M
    unsigned char* reconstruida = new unsigned char[totalPixeles];
    for(int i = 0; i < totalPixeles; i++) {
        reconstruida[i] = r_paso2[i] ^ aleatoria[i];
    }
    guardarImagen(reconstruida, ancho, alto, archivoReconstruida);
    cout << "Reconstruccion final completada (guardado en " << archivoReconstruida.toStdString() << ")" << endl;


    if (*original == *reconstruido){

        cout << "\n Ambos archivos son iguales"<<endl;
    }else {
        cout << "\n El archivo reconstruido es diferente"<<endl;
    }


    // Liberar memoria
    delete[] original;
    delete[] aleatoria;
    delete[] paso1;
    delete[] paso2;
    delete[] paso3;
    delete[] r_paso1;
    delete[] r_paso2;
    delete[] reconstruida;
    delete[] datosMascara;
    delete[] reconstruido ;

    cout << "\nProceso completado exitosamente!" << endl;
    return 0;
}

// Implementación de funciones...

unsigned char* cargarImagen(QString archivo, int &ancho, int &alto) {
    QImage imagen(archivo);
    if(imagen.isNull()) {
        cout << "Error al cargar: " << archivo.toStdString() << endl;
        return nullptr;
    }

    imagen = imagen.convertToFormat(QImage::Format_RGB888);
    ancho = imagen.width();
    alto = imagen.height();

    int tamano = ancho * alto * 3;
    unsigned char* pixeles = new unsigned char[tamano];

    for(int y = 0; y < alto; y++) {
        memcpy(pixeles + y * ancho * 3, imagen.scanLine(y), ancho * 3);
    }

    return pixeles;
}

bool guardarImagen(unsigned char* pixeles, int ancho, int alto, QString archivo) {
    QImage imagen(ancho, alto, QImage::Format_RGB888);

    for(int y = 0; y < alto; y++) {
        memcpy(imagen.scanLine(y), pixeles + y * ancho * 3, ancho * 3);
    }

    if(!imagen.save(archivo, "BMP")) {
        cout << "Error al guardar: " << archivo.toStdString() << endl;
        return false;
    }
    return true;
}

unsigned char rotarDerecha3bits(unsigned char valor) {
    return (valor >> 3) | (valor << 5);
}

unsigned char rotarIzquierda3bits(unsigned char valor) {
    return (valor << 3) | (valor >> 5);
}

bool cargarDatosEnmascaramiento(const char* archivo, int &semilla, unsigned int* &datos, int &cantidad) {
    ifstream f(archivo);
    if(!f.is_open()) {
        cout << "Error al abrir: " << archivo << endl;
        return false;
    }

    // Leer semilla
    f >> semilla;

    // Contar cantidad de tripletas RGB
    cantidad = 0;
    int r, g, b;
    while(f >> r >> g >> b) {
        cantidad++;
    }

    // Volver a leer para cargar datos
    f.clear();
    f.seekg(0);
    f >> semilla;

    datos = new unsigned int[cantidad * 3];
    for(int i = 0; i < cantidad * 3; i += 3) {
        f >> r >> g >> b;
        datos[i] = r;
        datos[i+1] = g;
        datos[i+2] = b;
    }

    f.close();
    return true;
}
