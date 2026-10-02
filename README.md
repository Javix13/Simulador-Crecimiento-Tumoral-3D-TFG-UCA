# Simulador de un Crecimiento Tumoral 3D en Unity
Este es el repositorio del TFG del alumno Javier González Ramos estudiante de Ingeniería Informática en la Universidad de Cádiz.

Aquí tendrás a disposición los ficheros fuentes de Unity, C++ y Java.

## Estructura del Proyecto
Los principales directorios del proyecto son los siguientes:
* Assets: En este directorio encontraremos los distintos scripts, escenas, materiales y shaders que componen nuestro proyecto Unity.
* NoUIVersion: En este directorio se encuentran los ficheros fuente en C++ y Java del simulador.


## Manual de usuario

Las instrucciones de uso del software se detallan a continuación. Este manual está dirigido al usuario final del software desarrollado en este proyecto.

### Introducción

En este apartado se explica cómo preparar el entorno para poder utilizar el simulador en sus versiones de **C++**, **Java** y **Unity**.

Las versiones de **C++** y **Java** han sido probadas en:

* Windows 10
* Windows 11
* Ubuntu 26.04.1 LTS
* macOS Sequoia 15.5

La versión desarrollada en **Unity** está disponible únicamente para **Windows**.

---

### Instalación

Para poder utilizar las distintas versiones del simulador se deberán cumplir los siguientes requisitos:

* Compilador **g++ (GCC) 15.1.0 o superior**.
* Compilador **javac 21.0.3 o superior**.
* Sistema operativo **Windows** para el simulador desarrollado en Unity.

Una vez cumplidos los requisitos, solo será necesario extraer los archivos del fichero comprimido. Tras ello, estarán listos para su uso.

---

## Uso del sistema

En esta sección se explica cómo utilizar las distintas versiones del simulador.

Por comodidad, el manual se divide en subsecciones donde se describe de forma detallada el uso de cada una de las implementaciones.

### Versión en C++

Para utilizar la versión en C++, dirígete a la carpeta:

```text
NoUIVersion/C++
```

En esta carpeta se encuentran las versiones **secuencial** y **paralela** del algoritmo.

#### Compilación

##### Versión secuencial

Abre una terminal en la carpeta correspondiente y ejecuta:

```bash
g++ TumorPOOV2.cpp -o NombreEjecutable -std=c++20
```

##### Versión paralela

Para compilar la versión paralela:

```bash
g++ TumorPOOV2Paralelo.cpp -o NombreEjecutable -std=c++20
```

#### Ejecución

Una vez compilados los programas, pueden ejecutarse desde la terminal.

##### Versión secuencial
###### Windows
```bash
NombreEjecutable
```
###### Ubuntu / MacOS
```bash
./NombreEjecutable
```
##### Versión paralela
###### Windows
```bash
NombreEjecutable -th NumerodeHilos
```
###### Ubuntu / MacOS
```bash
./NombreEjecutable -th NumerodeHilos
```

Donde:

* `NombreEjecutable` es el nombre asignado al archivo ejecutable durante la compilación.
* `NumerodeHilos` es el número de hilos que se utilizarán durante la simulación.

---

### Versión en Java

Para utilizar la versión en Java, dirígete a la carpeta:

```text
NoUIVersion/Java
```

Esta carpeta contiene las versiones **secuencial** y **paralela** del algoritmo.

#### Compilación

##### Versión secuencial

```bash
javac tumorPOOV2.java
```

##### Versión paralela

```bash
javac tumorPOOParaleloV2.java
```

#### Ejecución

##### Versión secuencial

```bash
java tumorPOOV2
```

##### Versión paralela

```bash
java tumorPOOParaleloV2 numeroDeHilos
```

En la versión paralela se deberá indicar el número de hilos que se utilizarán para ejecutar la simulación.

---

## Versión en Unity

Para utilizar la versión desarrollada en Unity, extrae los archivos del simulador en cualquier carpeta y ejecuta el archivo:

```text
Simulador de Crecimiento Tumoral.exe
```

Una vez ejecutado, aparecerá la pantalla de carga de Unity y, posteriormente, el menú principal.

El simulador está compuesto por dos pantallas principales:

1. **Menú principal**, desde el que se pueden crear y seleccionar configuraciones.
2. **Pantalla de simulación**, donde se visualiza el crecimiento tumoral en tiempo real.

En las siguientes secciones se explica el funcionamiento de cada una de ellas y los controles disponibles durante la simulación.

---

### Menú principal

En el menú principal se encuentran los botones necesarios para comenzar una simulación de crecimiento neoplásico.

Los botones principales son:

* **Start**
* **Configuración**

#### Configuración

El botón **Configuración** permite personalizar los distintos parámetros que conforman la simulación.

En esta ventana se proporciona información detallada sobre:

* Los parámetros que pueden modificarse.
* Los rangos permitidos.
* Los valores válidos para cada parámetro.

Si el usuario introduce algún valor incorrecto o deja algún campo sin completar, el sistema mostrará un mensaje indicando el error.

Si todos los parámetros son correctos, se mostrará un mensaje de confirmación.

Posteriormente, desde el menú **Start**, será posible cargar la configuración creada en el simulador.

Las configuraciones se almacenan en formato **JSON** en la siguiente carpeta:

```text
AppData\LocalLow\JavierGonzalezRamos\Simulador de Crecimiento Tumoral
```

---

### Simulación

Al pulsar el botón **Comenzar**, aparecerá una nueva pantalla donde se podrá observar la evolución de la simulación.

Durante la simulación aparecerán esferas que se desplazan, cambian de color y desaparecen. Estas esferas representan las células tumorales en el espacio tridimensional.

La representación de las células utiliza diferentes colores:

* **Rojo**: células madre o células con el potencial máximo de proliferación.
* **Blanco**: células no madre cuyo potencial de proliferación se encuentra a la mitad.
* **Negro**: células no madre cerca de la exhaustación.

Además, la interfaz permite visualizar información sobre el estado actual de la simulación, incluyendo:

* El día actual de la simulación.
* El número de células madre vivas.
* El número de células no madre vivas.

#### Controles de cámara

El simulador permite mover y rotar la cámara para observar el crecimiento tumoral desde diferentes posiciones.

| Tecla | Acción                                                 |
| ----- | ------------------------------------------------------ |
| `W`   | Mueve la cámara hacia arriba                           |
| `S`   | Mueve la cámara hacia abajo                            |
| `A`   | Mueve la cámara hacia la izquierda                     |
| `D`   | Mueve la cámara hacia la derecha                       |
| `Q`   | Rota la cámara hacia la izquierda y cambia su posición |
| `E`   | Rota la cámara hacia la derecha y cambia su posición   |

### Pausar la simulación

Es posible pausar la simulación pulsando la tecla:

```text
Espacio
```

Al pausar la simulación, el crecimiento tumoral se detiene, pero el movimiento de la cámara continúa disponible para poder observar el estado actual del tumor.

### Salir de la simulación

Mientras la simulación está en pausa, pulsa:

```text
Escape
```

para volver al menú principal.
