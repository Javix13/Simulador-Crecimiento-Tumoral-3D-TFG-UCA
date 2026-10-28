#include <array>
#include <atomic>
#include <barrier>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
#include <list>

// Clase para representar una célula
struct Cell {
  int x, y, z;
  bool isStemCell;
  int ro;

  Cell(int x, int y, int z, bool isStemCell, int ro)
      : x(x), y(y), z(z), isStemCell(isStemCell), ro(ro) {}
};

// Variables globales
int N_THREADS = 12; // por defecto
const int nDays = 120;
const int nSteps = 24 * nDays;
const int min_cells = 0, max_cells = 0;
const int gridSize = 500;
int cellSize = gridSize / N_THREADS;
bool grid[gridSize][gridSize][gridSize] = {{{false}}};
std::vector<std::vector<Cell>> threadCells;
std::vector<std::vector<Cell>> threadTempCells;
double runTime = 0;
std::chrono::time_point<std::chrono::high_resolution_clock> start, end;

// Variables globales para la concurrencia
std::vector<std::thread> threads;
std::vector<std::array<std::array<int, 3>, 2>> threadCoorsInDangerZone;

std::vector<std::unique_ptr<std::mutex>> threadDangerZoneMutexes;
std::vector<std::unique_ptr<std::mutex>> threadCellsMutexes;
std::atomic<int> stemcellsAlive = 0;
std::atomic<int> nonStemcellsAlive = 0;
std::atomic<int> cellsAlive = 0;
std::barrier<> *barrier;




// Parámetros de simulación
const double pd = 1.0 / 24.0 * 1.30;
const double pm = 10.0 / 24.0;
const double ps = 0.1;
const int rhomax = 10;
const double alpha = 0.01;

/**
 * @brief cuenta el número de células en la matriz
 * Función que se encarga de contar y comparar el número de células de la matriz con el número de células vivas
 * con el fin de comprobar que no ha habido problemas de concurrencia que hayan derivado a condiciones de carrera que produzcan
 * una desincronización en el conteo.
 */
void countCellsInGrid() 
{
  int count = 0;
  for (int i = 0; i < gridSize; ++i) 
  {
    for (int j = 0; j < gridSize; ++j) 
    {
      for (int k = 0; k < gridSize; ++k) 
      {
        if (grid[i][j][k]) 
        {
          ++count;
        }
      }
    }
  }
  std::cout << "Total cells in grid: " << count << "\n";
  if (count == cellsAlive) {
    std::cout << "The number of cells in the grid matches cellsAlive: " << count
              << "\n";
  } else {
    std::cout << "Mismatch! Number of cells in the grid: " << count
              << ", cellsAlive: " << cellsAlive << "\n";
    system("pause");
  }
}
/**
 * @brief Protocolo de salida de la zona de peligro
 * @param thread_id id del hilo actual
 * @param mutexID id del mutex utilizado
 */
void leaveDangerZone(int thread_id, int mutexID) {
  if (mutexID != -1) {
    threadDangerZoneMutexes[mutexID]->lock();
    threadCoorsInDangerZone[mutexID][mutexID == thread_id ? 0 : 1] = {-1, -1,
                                                                      -1};
    threadDangerZoneMutexes[mutexID]->unlock();
  }
}

/**
 * @brief Protocolo de entrada a la zona de peligro
 * El funcionamiento del protocolo está desrito en las secciones 2.6 y 5.4 del documento del TFG
 * @param x posición X de la célula
 * @param y posición y de la célula
 * @param z posición z de la célula
 * @param start_index indice inicial de la zona de trabajo del hilo
 * @param end_index indice final de la zona de trabajo del hilo
 * @param thread_id id del hilo actual
 * @param mutexID párametro de entrada y salida para guardar el id del mutex usado por el hilo actual
 * @return El hilo actual se encuentra próximo a su vecino
 */
bool InDangerZone(int x, int y, int z, int start_index, int end_index,
                  int thread_id, int *mutexID) {
  if (thread_id != 0 && thread_id != N_THREADS - 1) {

    if (x <= start_index + 1) {
      *mutexID = thread_id - 1;
      threadDangerZoneMutexes[*mutexID]->lock();
      auto threadCellcoor = threadCoorsInDangerZone[*mutexID][0];
      if (threadCellcoor[0] != -1 && abs(threadCellcoor[0] - x) <= 2 &&
          abs(threadCellcoor[1] - y) <= 2 && abs(threadCellcoor[2] - z) <= 2) {
        threadDangerZoneMutexes[*mutexID]->unlock();
        return true;
      } else {
        threadCoorsInDangerZone[*mutexID][1] = {x, y, z};
        threadDangerZoneMutexes[*mutexID]->unlock();
        return false;
      }
    }
    if (x >= end_index - 1) {
      *mutexID = thread_id;
      threadDangerZoneMutexes[*mutexID]->lock();
      auto threadCellcoor = threadCoorsInDangerZone[*mutexID][1];
      if (threadCellcoor[0] != -1 && abs(threadCellcoor[0] - x) <= 2 &&
          abs(threadCellcoor[1] - y) <= 2 && abs(threadCellcoor[2] - z) <= 2) {
        threadDangerZoneMutexes[*mutexID]->unlock();
        return true;
      } else {
        threadCoorsInDangerZone[*mutexID][0] = {x, y, z};
        threadDangerZoneMutexes[*mutexID]->unlock();
        return false;
      }
    } else
      return false;
  } else {

    if (thread_id == 0) {
      *mutexID = thread_id;
      if (x < end_index - 1) {
        return false;
      }
      threadDangerZoneMutexes[*mutexID]->lock();

      auto threadCellcoor = threadCoorsInDangerZone[*mutexID][1];
      if (threadCellcoor[0] != -1 && abs(threadCellcoor[0] - x) <= 2 &&
          abs(threadCellcoor[1] - y) <= 2 && abs(threadCellcoor[2] - z) <= 2) {
        threadDangerZoneMutexes[*mutexID]->unlock();
        return true;
      } else {
        threadCoorsInDangerZone[*mutexID][0] = {x, y, z};
        threadDangerZoneMutexes[*mutexID]->unlock();
        return false;
      }
    } else {

      *mutexID = thread_id - 1;

      if (x > start_index + 1) {
        return false;
      }
      threadDangerZoneMutexes[*mutexID]->lock();
      auto threadCellcoor = threadCoorsInDangerZone[*mutexID][0];
      if (threadCellcoor[0] != -1 && abs(threadCellcoor[0] - x) <= 2 &&
          abs(threadCellcoor[1] - y) <= 2 && abs(threadCellcoor[2] - z) <= 2) {
        threadDangerZoneMutexes[*mutexID]->unlock();
        return true;
      } else {
        threadCoorsInDangerZone[*mutexID][1] = {x, y, z};
        threadDangerZoneMutexes[*mutexID]->unlock();
        return false;
      }
    }
  }
}

/**
 * @brief Obtiene los vecinos vacios de una célula siguiendo la vecindad de Moore
 *
 * @param x Coordenada X de la célula
 * @param y Coordenada Y de la célula
 * @param z Coordenada Z de la célula
 * @return Lista de coordenadas de vecinos vacios
 */
std::vector<std::array<int, 3>> getEmptyNeighbors(int i, int j, int k) {
  std::vector<std::array<int, 3>> neighbors;

  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      for (int z = -1; z <= 1; ++z) {
        if (x == 0 && y == 0 && z == 0)
          continue;

        int ni = i + x, nj = j + y, nk = k + z;
        if (ni >= 0 && ni < gridSize && nj >= 0 && nj < gridSize && nk >= 0 &&
            nk < gridSize && !grid[ni][nj][nk]) {
          neighbors.push_back({ni, nj, nk});
        }
      }
    }
  }
  return neighbors;
}

/**
 * @brief Actualiza el estado de una célula madre
 * Sigue la función de transición descrita en 2.5.4 del documento del TFG y el protocolo de exclusión mutua
 * 
 * @param thread_id id del hilo actual
 * @param start_index indice inicial de la zona de trabajo del hilo
 * @param end_index indice final de la zona de trabajo del hilo
 * @param cell célula madre a procesar
 * @param generator generador de números aleatorios común a todos los hilos
 * @param distribution distribución utilizada para generar los números aleatorios
 */
void updateStemCells(int thread_id, int start_index, int end_index, Cell cell,
                     std::default_random_engine &generator,
                     std::uniform_real_distribution<double> &distribution) {
  int mutexID = -1;
  if (InDangerZone(cell.x, cell.y, cell.z, start_index, end_index, thread_id,
                   &mutexID)) {
    threadCells[thread_id].emplace_back(cell);
    return;
  }

  auto neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);
  if (!neighbors.empty()) {
    double fate = distribution(generator);
    std::uniform_int_distribution<size_t> neighbor_dist(0, neighbors.size() - 1);
    auto newCellPosition = neighbors[neighbor_dist(generator)];
    
    if (fate < pd) {
      fate = distribution(generator);
      if (fate <= ps) {

        grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
        int vectorIndex =
            std::min(newCellPosition[0] / cellSize, N_THREADS - 1);

        threadCellsMutexes[vectorIndex]->lock();
        threadTempCells[vectorIndex].emplace_back(
            Cell(newCellPosition[0], newCellPosition[1], newCellPosition[2],
                 true, rhomax));
        threadCellsMutexes[vectorIndex]->unlock();

        threadCellsMutexes[thread_id]->lock();
        threadTempCells[thread_id].emplace_back(
            Cell(cell.x, cell.y, cell.z, true, cell.ro));
        threadCellsMutexes[thread_id]->unlock();

        stemcellsAlive++;
        cellsAlive++;
      } else {

        grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
        int vectorIndex =
            std::min(newCellPosition[0] / cellSize, N_THREADS - 1);

        threadCellsMutexes[vectorIndex]->lock();
        threadTempCells[vectorIndex].emplace_back(
            Cell(newCellPosition[0], newCellPosition[1], newCellPosition[2],
                 false, rhomax));
        threadCellsMutexes[vectorIndex]->unlock();

        threadCellsMutexes[thread_id]->lock();
        threadTempCells[thread_id].emplace_back(
            Cell(cell.x, cell.y, cell.z, true, cell.ro));
        threadCellsMutexes[thread_id]->unlock();

        nonStemcellsAlive++;
        cellsAlive++;
      }
    } else if (fate < pm) {

      auto newCoor = neighbors[rand() % neighbors.size()];
      grid[newCoor[0]][newCoor[1]][newCoor[2]] = true;
      grid[cell.x][cell.y][cell.z] = false;
      int vectorIndex = std::min(newCoor[0] / cellSize, N_THREADS - 1);
      threadCellsMutexes[vectorIndex]->lock();
      threadTempCells[vectorIndex].emplace_back(
          Cell(newCoor[0], newCoor[1], newCoor[2], true, rhomax));
      threadCellsMutexes[vectorIndex]->unlock();
    } else {
      threadCellsMutexes[thread_id]->lock();
      threadTempCells[thread_id].emplace_back(cell);
      threadCellsMutexes[thread_id]->unlock();
    }
  } else {
    threadCellsMutexes[thread_id]->lock();
    threadTempCells[thread_id].emplace_back(cell);
    threadCellsMutexes[thread_id]->unlock();
  }
  leaveDangerZone(thread_id, mutexID);
}



/**
 * @brief Actualiza el estado de una célula no madre
 * siguiendo la función de transición descrita en 2.5.4 del documento del TFG y el protocolo de exclusión mutua
 *
 * @param thread_id id del hilo actual
 * @param start_index indice inicial de la zona de trabajo del hilo
 * @param end_index indice final de la zona de trabajo del hilo
 * @param cell célula madre a procesar
 * @param generator generador de números aleatorios común a todos los hilos
 * @param distribution distribución utilizada para generar los números aleatorios
 */
void updateNonStemCells(int thread_id, int start_index, int end_index,
                        Cell cell, std::default_random_engine &generator,
                        std::uniform_real_distribution<double> &distribution) {
  int mutexID = -1;
  if (InDangerZone(cell.x, cell.y, cell.z, start_index, end_index, thread_id,
                   &mutexID)) {
    threadCells[thread_id].emplace_back(cell);
    return;
  }
  double r = distribution(generator);

  if (r < alpha) {
    grid[cell.x][cell.y][cell.z] = false;
    nonStemcellsAlive--;
    cellsAlive--;
    leaveDangerZone(thread_id, mutexID);
    return;
  }
  auto neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);
  if (!neighbors.empty()) {
    double fate = distribution(generator);
    std::uniform_int_distribution<size_t> neighbor_dist(0, neighbors.size() - 1);
    auto newCellPosition = neighbors[neighbor_dist(generator)];

    if (fate < pd) {
      if (cell.ro == 0) {
        grid[cell.x][cell.y][cell.z] = false;
        nonStemcellsAlive--;
        cellsAlive--;
      } else {
        cell.ro--;
        grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
        int vectorIndex =
            std::min(newCellPosition[0] / cellSize, N_THREADS - 1);
        threadCellsMutexes[vectorIndex]->lock();
        threadTempCells[vectorIndex].emplace_back(
            Cell(newCellPosition[0], newCellPosition[1], newCellPosition[2],
                 false, cell.ro));
        threadCellsMutexes[vectorIndex]->unlock();

        threadCellsMutexes[thread_id]->lock();
        threadTempCells[thread_id].emplace_back(cell);
        threadCellsMutexes[thread_id]->unlock();

        nonStemcellsAlive++;
        cellsAlive++;
      }
    } else if (fate < pm) {
      grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
      grid[cell.x][cell.y][cell.z] = false;
      int vectorIndex = std::min(newCellPosition[0] / cellSize, N_THREADS - 1);

      threadCellsMutexes[vectorIndex]->lock();
      threadTempCells[vectorIndex].emplace_back(
          Cell(newCellPosition[0], newCellPosition[1], newCellPosition[2],
               false, cell.ro));
      threadCellsMutexes[vectorIndex]->unlock();
    } else {

      threadCellsMutexes[thread_id]->lock();
      threadTempCells[thread_id].emplace_back(cell);
      threadCellsMutexes[thread_id]->unlock();
    }
  } else {

    threadCellsMutexes[thread_id]->lock();
    threadTempCells[thread_id].emplace_back(cell);
    threadCellsMutexes[thread_id]->unlock();
  }
  leaveDangerZone(thread_id, mutexID);
}


/**
 * @brief Actualiza el estado del autómata celular en el hilo actual
 *  
 * @param thread_id id del hilo actual
 * @param start_index indice inicial de la zona de trabajo del hilo
 * @param end_index indice final de la zona de trabajo del hilo
 * @param cell célula madre a procesar
 * @param generator generador de números aleatorios común a todos los hilos
 * @param distribution distribución utilizada para generar los números aleatorios
 */
void updateState(int thread_id, int start_index, int end_index,
                 std::default_random_engine &generator,
                 std::uniform_real_distribution<double> &distribution) {
  std::vector<Cell>& threadVector =  threadCells[thread_id];
  while(!threadVector.empty()){
    unsigned index = rand() % threadVector.size();
    std::swap(threadVector[index],threadVector.back());
    Cell cell = threadVector.back();
    threadVector.pop_back();
     if (cell.isStemCell) {
      updateStemCells(thread_id, start_index, end_index, cell, generator,
                      distribution);
    } else {
      updateNonStemCells(thread_id, start_index, end_index, cell, generator,
                         distribution);
    }
}}

/**
 * @brief Ejecuta la simulación paralela del crecimiento tumoral.
 * Cada hilo itera durante el número total de pasos de simulación, actualizando
 * el estado de las células en cada paso. Cuando todos los hilos terminan de procesar
 * su parte, el hilo 0 muestra por pantalla el estado actual de la simulación y
 * tras cada actualización cada hilo intercambia el estado actual con el
 * temporal y limpia el contenedor temporal para el siguiente paso.
 * @param thread_id id del hilo actual
 * @param start_index indice inicial de la zona de trabajo del hilo
 * @param end_index indice final de la zona de trabajo del hilo
 */

void parallel_simulation(int thread_id, int start_index, int end_index) {
  std::default_random_engine generator(time(0));
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  for (int step = 1; step <= nSteps; ++step) {
    updateState(thread_id, start_index, end_index, generator, distribution);
    barrier->arrive_and_wait();
    if (thread_id == 0 && step % 24 == 0) {
      std::chrono::time_point<std::chrono::high_resolution_clock> elapsedTime;
      elapsedTime = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsedSecondsDuration = elapsedTime-start;
      std::cout << "Day: " << step / 24 << "\n";
      std::cout << "cellsAlive: " << cellsAlive << "\n";
      std::cout << "Elapsed time: " << elapsedSecondsDuration.count() << "s\n";
    }
    threadCells[thread_id].swap(threadTempCells[thread_id]);
    threadTempCells[thread_id].clear();
    barrier->arrive_and_wait();
  }
}


/**
 * @brief Verifica que el número de células totales coincide con la suma de las células almacenadas en los vectores
 * de trabajo de cada hilo y lo muestra por pantalla.
 */
void verifyCellCount() {
  int totalCells = 0;
  int thread = 0;
  for (const auto &cells : threadCells) {
    std::cout << "Cells in thread " << thread << ": " << cells.size()
              << std::endl;
    totalCells += cells.size();
    std::cout<<totalCells<<std::endl;
    thread++;
  }
  if (totalCells == cellsAlive) {
    std::cout << "The cell count matches: " << totalCells << "\n";
  } else {
    std::cout << "Mismatch in cell count. Total cells: " << totalCells
              << ", cellsAlive: " << cellsAlive << "\n";
  }
}

/**
 * @brief Guarda los datos de la simulación en un archivo de texto.
 *
 * Abre el archivo de resultados en modo de anexado y escribe el número
 * de células vivas y el tiempo de ejecución, separados por dos puntos.
 * El nombre del archivo se genera a partir del número de hilos, tamaño de la matriz
 * y del número de días de simulación.
 *
 * Si el archivo no puede abrirse, muestra un mensaje de error por consola.
 *
 * @note Cada registro se añade en una línea nueva, conservando los datos
 *       almacenados previamente en el archivo.
 */
void appendSimulationData() {
  std::ofstream file("simdata" + std::to_string(N_THREADS) + "threads" +
                         std::to_string(gridSize) + "grid" +
                         std::to_string(nDays) + "days.txt",
                     std::ios::app);
  if (file.is_open()) {
    file << cellsAlive << ":" << runTime << "\n";
    file.close();
  } else {
    std::cerr << "Error opening file for appending data.\n";
  }
}


/**
 * @brief Inicializa las variables globales utilizadas en el algoritmo
 */
void initializeGlobals() {

  // Redimensionar los vectores según el número de hilos
  threads.resize(N_THREADS);
  threadCoorsInDangerZone.resize(N_THREADS - 1, {{{-1, -1, -1}, {-1, -1, -1}}});
  threadDangerZoneMutexes.reserve(N_THREADS - 1);

  for (size_t i = 0; i < N_THREADS - 1; ++i) {
    threadDangerZoneMutexes.push_back(std::make_unique<std::mutex>());
  }
  threadCellsMutexes.reserve(N_THREADS);
  for (size_t i = 0; i < N_THREADS; ++i) {
    threadCellsMutexes.push_back(std::make_unique<std::mutex>());
  }
  threadCells.resize(N_THREADS);
  threadTempCells.resize(N_THREADS);

  // Recalcular el tamaño de las celdas
  cellSize = gridSize / N_THREADS;

  // Crear la barrera
  barrier = new std::barrier<>(N_THREADS);
}


/**
 * @brief Inicializa y ejecuta la simulación paralela del crecimiento tumoral.
 *
 * Configura las variables globales y coloca la célula madre inicial en el
 * centro de la cuadrícula tridimensional, asignándola al vector de células
 * del hilo correspondiente.
 *
 * Crea los hilos de ejecución y distribuye entre ellos los intervalos de
 * coordenadas del eje X de la cuadrícula. Cada hilo ejecuta la función
 * parallel_simulation() sobre su región asignada.
 *
 * Espera a que todos los hilos finalicen y calcula el tiempo total de
 * ejecución. A continuación, muestra las estadísticas de células vivas,
 * comprueba la consistencia del recuento de células y libera la barrera
 * de sincronización.
 *
 * Si el número de células vivas no cumple los límites establecidos por
 * min_cells y max_cells, descarta los resultados y finaliza el programa.
 * En caso contrario, guarda los resultados mediante appendSimulationData().
 *
 * @param argc Número de argumentos recibidos por línea de comandos.
 * @param argv Array de argumentos recibidos por línea de comandos.
 *             El segundo argumento (argv[2]) especifica el número de hilos.
 *
 * @return 0 al finalizar la ejecución.
 */
int main(int argc, char *argv[]) {
  if (argc > 1) {
    std::cout << "Number of threads: " << argv[2] << "\n";
    N_THREADS = std::stoi(argv[2]);
  }
  initializeGlobals();
  grid[gridSize / 2][gridSize / 2][gridSize / 2] = true;
  threadCells[N_THREADS / 2].emplace_back(gridSize / 2, gridSize / 2,
                                          gridSize / 2, true, rhomax);
  stemcellsAlive = 1;
  cellsAlive = 1;
  // Crear los threads
   start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < N_THREADS; ++i) {
    if (i == N_THREADS - 1)
      threads[i] = std::thread(parallel_simulation, i,
                               i * (gridSize / N_THREADS), gridSize - 1);

    else
      threads[i] =
          std::thread(parallel_simulation, i, i * (gridSize / N_THREADS),
                      (i + 1) * (gridSize / N_THREADS) - 1);
  }
  for (auto &thread : threads) {
    thread.join();
  }
  end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> runTimeDuration = end-start;

  runTime= runTimeDuration.count();
  std::cout << "stemcellsAlive: " << stemcellsAlive << "\n";
  std::cout << "nonStemcellsAlive: " << nonStemcellsAlive << "\n";
  std::cout << "cellsAlive: " << cellsAlive << "\n";
  std::cout << "Total time: " << runTime << "s\n";
  std::cout << "Number of threads: " << N_THREADS << std::endl;
  countCellsInGrid();
  verifyCellCount();
  delete barrier;
  if ((min_cells != 0 && cellsAlive < min_cells) ||
      (max_cells != 0 && cellsAlive > max_cells)) {
    std::cout << "Simulation discarded, cells alive: " << cellsAlive
              << std::endl;

    return 0;
  }
  appendSimulationData();
  return 0;
}
