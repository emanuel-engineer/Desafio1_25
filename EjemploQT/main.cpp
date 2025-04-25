#include <fstream>
#include <iostream>
#include <QCoreApplication>
#include <QImage>
#include <QDir>
#include <QFileInfo>
#include <cstring>

using namespace std;

// Definición de posibles transformaciones
enum TipoTransformacion {
    XOR_CON_IM,
    ROTACION_DERECHA,
    ROTACION_IZQUIERDA,
    DESPLAZAMIENTO_DERECHA,
    DESPLAZAMIENTO_IZQUIERDA,
    NINGUNA // Para casos especiales
};

// Estructura para almacenar información de transformación
struct Transformacion {
    TipoTransformacion tipo;
    int bits; // Número de bits para rotación/desplazamiento
};

// Prototipos de funciones
unsigned char* cargarImagen(QString archivo, int &ancho, int &alto);
bool guardarImagen(unsigned char* pixeles, int ancho, int alto, QString archivo);
unsigned char rotarDerecha(unsigned char valor, int bits);
unsigned char rotarIzquierda(unsigned char valor, int bits);
unsigned char desplazarDerecha(unsigned char valor, int bits);
unsigned char desplazarIzquierda(unsigned char valor, int bits);
bool cargarDatosEnmascaramiento(const QString& archivo, int &semilla, unsigned int* &datos, int &cantidad);
bool verificarEnmascaramiento(unsigned char* imagen, int anchoImagen, int altoImagen,
const QString& archivoMascara, const QString& rutaSalida);
void aplicarTransformacion(unsigned char* entrada, unsigned char* salida, int totalPixeles,
Transformacion trans, unsigned char* imagenAuxiliar = nullptr);
void aplicarTransformacionInversa(unsigned char* entrada, unsigned char* salida, int totalPixeles,
Transformacion trans, unsigned char* imagenAuxiliar = nullptr);
bool compararImagenes(unsigned char* img1, unsigned char* img2, int totalPixeles);
Transformacion detectarTransformacion(unsigned char* antes, unsigned char* despues,
unsigned char* imagenAuxiliar, int totalPixeles);
void reconstruirImagen(const QString& casoDirectorio, const QString& dirSalida);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Directorios base (se pueden pasar como parámetros)
    QString dirBase = QDir::currentPath();
    QString dirSalida = dirBase + "/salida";

    // Crear directorio de salida si no existe
    QDir().mkpath(dirSalida);

    // Iterar sobre casos disponibles
    QDir dirCasos(dirBase + "/casos");
    QStringList casos = dirCasos.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (casos.isEmpty()) {
        cout << "No se encontraron casos para procesar en " << dirCasos.absolutePath().toStdString() << endl;
        cout << "Por favor asegúrese de tener los directorios de casos correctamente ubicados." << endl;

        // Fallback a un caso específico si no se encuentran casos
        QString casoPath = dirBase + "/Desafio1_25/EjemploQT/caso1";
        if (QDir(casoPath).exists()) {
            cout << "Utilizando caso específico en: " << casoPath.toStdString() << endl;
            reconstruirImagen(casoPath, dirSalida);
        } else {
            cout << "No se encontró un caso específico para procesar." << endl;
            return 1;
        }
    } else {
        // Procesar cada caso encontrado
        for (const QString& caso : casos) {
            QString rutaCaso = dirCasos.absolutePath() + "/" + caso;
            cout << "\n\n========= Procesando caso: " << caso.toStdString() << " =========" << endl;
            reconstruirImagen(rutaCaso, dirSalida + "/" + caso);
        }
    }

    cout << "\n\nTodos los casos han sido procesados exitosamente!" << endl;
    return 0;
}

void reconstruirImagen(const QString& casoDirectorio, const QString& dirSalida)
{
    QDir().mkpath(dirSalida);

    // Encontrar archivos de entrada
    QString archivoOriginal, archivoTransformado, archivoAleatoria;
    QStringList mascaras;

    QDir dir(casoDirectorio);
    QStringList archivos = dir.entryList(QStringList() << "*.bmp" << "*.BMP", QDir::Files);

    for (const QString& archivo : archivos) {
        QString rutaCompleta = dir.absolutePath() + "/" + archivo;
        if (archivo.contains("_D", Qt::CaseInsensitive) || archivo.contains("_O", Qt::CaseInsensitive)) {
            archivoOriginal = rutaCompleta;
        } else if (archivo.contains("_M", Qt::CaseInsensitive)) {
            archivoAleatoria = rutaCompleta;
        }
    }

    // Encontrar archivos de máscara
    QStringList archivosMascara = dir.entryList(QStringList() << "M*.txt" << "m*.txt", QDir::Files);
    for (const QString& mascara : archivosMascara) {
        mascaras.append(dir.absolutePath() + "/" + mascara);
    }

    if (archivoOriginal.isEmpty() || archivoAleatoria.isEmpty() || mascaras.isEmpty()) {
        cout << "Error: No se encontraron los archivos necesarios" << endl;
        return;
    }

    // Ordenar mascaras por número (M1.txt, M2.txt, etc.)
    std::sort(mascaras.begin(), mascaras.end(), [](const QString& a, const QString& b) {
        QFileInfo infoA(a);
        QFileInfo infoB(b);
        QString numA = infoA.baseName().mid(1); // Quitar la M
        QString numB = infoB.baseName().mid(1);
        return numA.toInt() < numB.toInt();
    });

    cout << "Archivos encontrados:" << endl;
    cout << "Original: " << archivoOriginal.toStdString() << endl;
    cout << "Aleatoria: " << archivoAleatoria.toStdString() << endl;
    cout << "Mascaras (" << mascaras.size() << "):" << endl;
    for (const QString& m : mascaras) {
        cout << "  - " << m.toStdString() << endl;
    }

    // Cargar imágenes
    int ancho = 0, alto = 0;
    int ancho2 = 0, alto2 = 0;

    cout << "\nCargando imágenes..." << endl;
    unsigned char *original = cargarImagen(archivoOriginal, ancho, alto);
    unsigned char *aleatoria = cargarImagen(archivoAleatoria, ancho2, alto2);

    if(!original || !aleatoria || ancho != ancho2 || alto != alto2) {
        cout << "Error: Las imágenes no coinciden en dimensiones o no se pudieron cargar" << endl;
        return;
    }

    int totalPixeles = ancho * alto * 3;

    // La idea es reconstruir la secuencia de transformaciones aplicadas
    // Sabemos que después de cada transformación (excepto la última) se aplicó un enmascaramiento
    // Y tenemos los archivos M1.txt, M2.txt, etc.

    cout << "\nAnalizando posibles transformaciones..." << endl;

    // Vamos a probar diferentes secuencias de transformaciones usando los archivos de máscara
    // como guía para determinar cuáles fueron aplicadas.

    // Crear espacio para imágenes intermedias
    int numPasos = mascaras.size();
    unsigned char** pasos = new unsigned char*[numPasos + 1]; // +1 para la imagen final

    for (int i = 0; i <= numPasos; i++) {
        pasos[i] = new unsigned char[totalPixeles];
    }

    // Inicializar con la imagen transformada final
    memcpy(pasos[numPasos], original, totalPixeles);

    // Vector para almacenar la secuencia de transformaciones detectadas
    vector<Transformacion> secuenciaTransformaciones;

    // Empezamos desde la última máscara y vamos hacia atrás
    for (int i = numPasos - 1; i >= 0; i--) {
        cout << "Procesando paso " << i+1 << " usando mascara: " << mascaras[i].toStdString() << endl;

        // Cargar datos de enmascaramiento
        int semilla = 0;
        unsigned int* datosMascara = nullptr;
        int cantidadPixelesMascara = 0;

        if(!cargarDatosEnmascaramiento(mascaras[i], semilla, datosMascara, cantidadPixelesMascara)) {
            cout << "Error al cargar el archivo de enmascaramiento: " << mascaras[i].toStdString() << endl;
            continue;
        }
        cout << "  Datos de enmascaramiento cargados. Semilla: " << semilla << ", Píxeles: " << cantidadPixelesMascara << endl;

        // Ahora necesitamos determinar qué transformación se aplicó en este paso
        // Probamos todas las transformaciones posibles y verificamos cuál coincide

        TipoTransformacion tiposTransf[] = {XOR_CON_IM, ROTACION_DERECHA, ROTACION_IZQUIERDA,
                                          DESPLAZAMIENTO_DERECHA, DESPLAZAMIENTO_IZQUIERDA};
        int valoresBits[] = {0, 1, 2, 3, 4, 5, 6, 7};  // 0 para XOR (no aplica)

        bool transformacionEncontrada = false;
        Transformacion transEncontrada;

        // Copiamos la imagen del paso actual para experimentar
        unsigned char* imgPrueba = new unsigned char[totalPixeles];
        memcpy(imgPrueba, pasos[i+1], totalPixeles);

        // Aplicar desmacara primero (inverso del enmascaramiento)
        for(int j = 0; j < cantidadPixelesMascara * 3; j += 3) {
            int posicion = (semilla + j) % totalPixeles;
            // Restar los valores de la máscara (operación inversa de la suma)
            imgPrueba[posicion] = datosMascara[j] - imgPrueba[posicion];
            imgPrueba[posicion+1] = datosMascara[j+1] - imgPrueba[posicion+1];
            imgPrueba[posicion+2] = datosMascara[j+2] - imgPrueba[posicion+2];
        }

        // Probar cada transformación
        for (TipoTransformacion tipo : tiposTransf) {
            // Para tipos que requieren bits, probar diferentes valores
            int maxBits = (tipo == XOR_CON_IM) ? 1 : 8;  // XOR no usa el parámetro bits

            for (int bits = 0; bits < maxBits; bits++) {
                // Saltarse combinaciones innecesarias
                if (tipo == XOR_CON_IM && bits > 0) continue;

                unsigned char* resultado = new unsigned char[totalPixeles];
                Transformacion trans = {tipo, bits};

                // Aplicar transformación inversa
                aplicarTransformacionInversa(imgPrueba, resultado, totalPixeles, trans, aleatoria);

                // Guardar para debug y verificación
                QString nombrePaso = dirSalida + "/prueba_paso" + QString::number(i+1) +
                                    "_tipo" + QString::number(tipo) + "_bits" + QString::number(bits) + ".bmp";
                guardarImagen(resultado, ancho, alto, nombrePaso);

                // Simular la transformación directa para verificar
                unsigned char* verificacion = new unsigned char[totalPixeles];
                aplicarTransformacion(resultado, verificacion, totalPixeles, trans, aleatoria);

                // Aplicar enmascaramiento para verificar
                unsigned char* conMascara = new unsigned char[totalPixeles];
                memcpy(conMascara, verificacion, totalPixeles);

                for(int j = 0; j < cantidadPixelesMascara * 3; j += 3) {
                    int posicion = (semilla + j) % totalPixeles;
                    conMascara[posicion] = conMascara[posicion] + datosMascara[j];
                    conMascara[posicion+1] = conMascara[posicion+1] + datosMascara[j+1];
                    conMascara[posicion+2] = conMascara[posicion+2] + datosMascara[j+2];
                }

                // Si estamos en el paso final, comparar con la imagen original
                if (i == numPasos - 1 && compararImagenes(conMascara, pasos[i+1], totalPixeles)) {
                    cout << "  Transformación encontrada para paso " << i+1 << ": ";
                    switch(tipo) {
                        case XOR_CON_IM: cout << "XOR con imagen aleatoria"; break;
                        case ROTACION_DERECHA: cout << "Rotación derecha " << bits << " bits"; break;
                        case ROTACION_IZQUIERDA: cout << "Rotación izquierda " << bits << " bits"; break;
                        case DESPLAZAMIENTO_DERECHA: cout << "Desplazamiento derecha " << bits << " bits"; break;
                        case DESPLAZAMIENTO_IZQUIERDA: cout << "Desplazamiento izquierda " << bits << " bits"; break;
                        default: cout << "Desconocida"; break;
                    }
                    cout << endl;

                    transEncontrada = trans;
                    transformacionEncontrada = true;
                    memcpy(pasos[i], resultado, totalPixeles);
                    break;
                }

                delete[] resultado;
                delete[] verificacion;
                delete[] conMascara;
            }

            if (transformacionEncontrada) break;
        }

        if (transformacionEncontrada) {
            secuenciaTransformaciones.insert(secuenciaTransformaciones.begin(), transEncontrada);

            // Guardar la imagen reconstruida de este paso
            QString nombrePaso = dirSalida + "/paso_" + QString::number(i) + "_reconstruido.bmp";
            guardarImagen(pasos[i], ancho, alto, nombrePaso);
        } else {
            cout << "  No se pudo determinar la transformación para el paso " << i+1 << endl;
            // En este caso, podríamos usar una transformación nula o continuar con la mejor aproximación
            Transformacion transNula = {NINGUNA, 0};
            secuenciaTransformaciones.insert(secuenciaTransformaciones.begin(), transNula);
            memcpy(pasos[i], pasos[i+1], totalPixeles);  // Sin cambios
        }

        delete[] datosMascara;
        delete[] imgPrueba;
    }

    // Reconstrucción final de la imagen original
    unsigned char* imagenReconstruida = new unsigned char[totalPixeles];
    memcpy(imagenReconstruida, pasos[0], totalPixeles);

    // Guardar imagen reconstruida
    QString archivoReconstruida = dirSalida + "/imagen_reconstruida.bmp";
    guardarImagen(imagenReconstruida, ancho, alto, archivoReconstruida);
    cout << "\nImagen reconstruida guardada en: " << archivoReconstruida.toStdString() << endl;

    // Mostrar la secuencia de transformaciones encontrada
    cout << "\nSecuencia de transformaciones detectada (del primer al último paso):" << endl;
    for (size_t i = 0; i < secuenciaTransformaciones.size(); i++) {
        Transformacion t = secuenciaTransformaciones[i];
        cout << "Paso " << i+1 << ": ";
        switch(t.tipo) {
            case XOR_CON_IM: cout << "XOR con imagen aleatoria"; break;
            case ROTACION_DERECHA: cout << "Rotación derecha " << t.bits << " bits"; break;
            case ROTACION_IZQUIERDA: cout << "Rotación izquierda " << t.bits << " bits"; break;
            case DESPLAZAMIENTO_DERECHA: cout << "Desplazamiento derecha " << t.bits << " bits"; break;
            case DESPLAZAMIENTO_IZQUIERDA: cout << "Desplazamiento izquierda " << t.bits << " bits"; break;
            case NINGUNA: cout << "No identificada"; break;
        }
        cout << endl;
    }

    // Liberar memoria
    delete[] original;
    delete[] aleatoria;
    delete[] imagenReconstruida;

    for (int i = 0; i <= numPasos; i++) {
        delete[] pasos[i];
    }
    delete[] pasos;

    cout << "\nProceso completado para este caso!" << endl;
}

// Implementación de funciones auxiliares
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

unsigned char rotarDerecha(unsigned char valor, int bits) {
    return (valor >> bits) | (valor << (8 - bits));
}

unsigned char rotarIzquierda(unsigned char valor, int bits) {
    return (valor << bits) | (valor >> (8 - bits));
}

unsigned char desplazarDerecha(unsigned char valor, int bits) {
    return valor >> bits;
}

unsigned char desplazarIzquierda(unsigned char valor, int bits) {
    return valor << bits;
}

bool cargarDatosEnmascaramiento(const QString& archivo, int &semilla, unsigned int* &datos, int &cantidad) {
    ifstream f(archivo.toStdString().c_str());
    if(!f.is_open()) {
        cout << "Error al abrir: " << archivo.toStdString() << endl;
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

void aplicarTransformacion(unsigned char* entrada, unsigned char* salida, int totalPixeles,
                          Transformacion trans, unsigned char* imagenAuxiliar) {
    switch(trans.tipo) {
        case XOR_CON_IM:
            if (!imagenAuxiliar) {
                cout << "Error: Se requiere imagen auxiliar para XOR" << endl;
                memcpy(salida, entrada, totalPixeles);
                return;
            }
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = entrada[i] ^ imagenAuxiliar[i];
            }
            break;
        case ROTACION_DERECHA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = rotarDerecha(entrada[i], trans.bits);
            }
            break;
        case ROTACION_IZQUIERDA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = rotarIzquierda(entrada[i], trans.bits);
            }
            break;
        case DESPLAZAMIENTO_DERECHA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = desplazarDerecha(entrada[i], trans.bits);
            }
            break;
        case DESPLAZAMIENTO_IZQUIERDA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = desplazarIzquierda(entrada[i], trans.bits);
            }
            break;
        case NINGUNA:
        default:
            memcpy(salida, entrada, totalPixeles);
            break;
    }
}

void aplicarTransformacionInversa(unsigned char* entrada, unsigned char* salida, int totalPixeles,
                                 Transformacion trans, unsigned char* imagenAuxiliar) {
    switch(trans.tipo) {
        case XOR_CON_IM:
            if (!imagenAuxiliar) {
                cout << "Error: Se requiere imagen auxiliar para XOR" << endl;
                memcpy(salida, entrada, totalPixeles);
                return;
            }
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = entrada[i] ^ imagenAuxiliar[i];
            }
            break;
        case ROTACION_DERECHA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = rotarIzquierda(entrada[i], trans.bits);  // Inversa de rotación derecha
            }
            break;
        case ROTACION_IZQUIERDA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = rotarDerecha(entrada[i], trans.bits);  // Inversa de rotación izquierda
            }
            break;
        case DESPLAZAMIENTO_DERECHA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = desplazarIzquierda(entrada[i], trans.bits);  // No es perfectamente inversa
            }
            break;
        case DESPLAZAMIENTO_IZQUIERDA:
            for(int i = 0; i < totalPixeles; i++) {
                salida[i] = desplazarDerecha(entrada[i], trans.bits);  // No es perfectamente inversa
            }
            break;
        case NINGUNA:
        default:
            memcpy(salida, entrada, totalPixeles);
            break;
    }
}

bool compararImagenes(unsigned char* img1, unsigned char* img2, int totalPixeles) {
    // Verificar si las imágenes son similares (pueden haber pequeñas diferencias por redondeo)
    int diferencias = 0;
    int umbralDiferencia = 3;  // Tolerancia para diferencias de valor
    int umbralTotalDiferencias = totalPixeles * 0.01;  // Permitir hasta 1% de píxeles diferentes

    for(int i = 0; i < totalPixeles; i++) {
        if(abs(img1[i] - img2[i]) > umbralDiferencia) {
            diferencias++;
            if(diferencias > umbralTotalDiferencias) {
                return false;
            }
        }
    }

    return true;
}

bool verificarEnmascaramiento(unsigned char* imagen, int anchoImagen, int altoImagen,
                             const QString& archivoMascara, const QString& rutaSalida) {
    // Cargar datos de enmascaramiento
    int semilla = 0;
    unsigned int* datosMascara = nullptr;
    int cantidadPixelesMascara = 0;

    if(!cargarDatosEnmascaramiento(archivoMascara, semilla, datosMascara, cantidadPixelesMascara)) {
        cout << "Error al cargar el archivo de enmascaramiento" << endl;
        return false;
    }

    int totalPixeles = anchoImagen * altoImagen * 3;
    unsigned char* imgVerificacion = new unsigned char[totalPixeles];
    memcpy(imgVerificacion, imagen, totalPixeles);

    // Aplicar enmascaramiento para verificar
    for(int i = 0; i < cantidadPixelesMascara * 3; i += 3) {
        int posicion = (semilla + i) % totalPixeles;
        imgVerificacion[posicion] = imgVerificacion[posicion] + datosMascara[i];
        imgVerificacion[posicion+1] = imgVerificacion[posicion+1] + datosMascara[i+1];
        imgVerificacion[posicion+2] = imgVerificacion[posicion+2] + datosMascara[i+2];
    }

    // Guardar imagen verificada
    guardarImagen(imgVerificacion, anchoImagen, altoImagen, rutaSalida);

    delete[] imgVerificacion;
    delete[] datosMascara;

    return true;
}
