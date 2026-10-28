#include <array>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <vector>

// Clase para representar una célula
struct Cell {
  int x, y, z;
  bool isStemCell;
  int ro;

  Cell(int x, int y, int z, bool isStemCell, int ro)
      : x(x), y(y), z(z), isStemCell(isStemCell), ro(ro) {}
};

// Variables globales
const int nDays = 120;
const int nSteps = 24 * nDays;
const int gridSize = 500;
bool grid[gridSize][gridSize][gridSize] = {{{false}}};
std::vector<Cell> cells;
std::vector<Cell> tempCells;
int stemcellsAlive = 0;
int nonStemcellsAlive = 0;
int cellsAlive = 0;
double runTime = 0;
std::chrono::time_point<std::chrono::high_resolution_clock> start, end;

// Parámetros de simulación
const double pd = 1.0 / 24.0 * 1.30;
const double pm = 10.0 / 24.0;
const double ps = 0.1;
const int rhomax = 10;
const double alpha = 0.01;
std::default_random_engine generator(std::time(0));
std::uniform_real_distribution<double> distribution(0.0, 1.0);

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
 * siguiendo la función de transición descrita en 2.5.4 del documento del TFG
 *
 * @param cell célula madre a procesar
 */
void updateStemCells(Cell cell) {
  auto neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);

  if (!neighbors.empty()) {
    double fate = distribution(generator);
    std::uniform_int_distribution<size_t> neighbor_dist(0, neighbors.size() - 1);
    auto newCellPosition = neighbors[neighbor_dist(generator)];
    if (fate < pd) {
      fate = distribution(generator);

      if (fate <= ps) { // division simetrica
        grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
        tempCells.emplace_back(Cell(newCellPosition[0], newCellPosition[1],
                                    newCellPosition[2], true, rhomax));
        tempCells.emplace_back(Cell(cell.x, cell.y, cell.z, true, cell.ro));
        ++stemcellsAlive;
        ++cellsAlive;
      } else { // division asimetrica
        grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
        tempCells.emplace_back(Cell(newCellPosition[0], newCellPosition[1],
                                    newCellPosition[2], false, rhomax));
        tempCells.emplace_back(Cell(cell.x, cell.y, cell.z, true, cell.ro));
        ++nonStemcellsAlive;
        ++cellsAlive;
      }
    } else if (fate < pm) { // migracion
      grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] =
          grid[cell.x][cell.y][cell.z];
      grid[cell.x][cell.y][cell.z] = false;
      tempCells.emplace_back(Cell(newCellPosition[0], newCellPosition[1],
                                  newCellPosition[2], true, rhomax));
    } else { // nada
      tempCells.emplace_back(cell);
    }
  } else { // nada
    tempCells.emplace_back(cell);
  }
}

/**
 * @brief Actualiza el estado de una célula no madre
 * siguiendo la función de transición descrita en 2.5.4 del documento del TFG
 *
 * @param cell célula no madre a procesar
 */
void updateNonStemCells(Cell cell) {
  double fate = distribution(generator);
  if (fate < alpha) {
    grid[cell.x][cell.y][cell.z] = false;
    --nonStemcellsAlive;
    --cellsAlive;
    return;
  }

  auto neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);

  if (!neighbors.empty()) {
    fate = distribution(generator);
    std::uniform_int_distribution<size_t> neighbor_dist(0, neighbors.size() - 1);
    auto newCellPosition = neighbors[neighbor_dist(generator)];
    if (fate < pd) {
      if (cell.ro == 0) {
        grid[cell.x][cell.y][cell.z] = false;
        --nonStemcellsAlive;
        --cellsAlive;
      } else {
        --cell.ro;

        grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
        tempCells.emplace_back(cell);
        tempCells.emplace_back(Cell(newCellPosition[0], newCellPosition[1],
                                    newCellPosition[2], false, cell.ro));
        ++nonStemcellsAlive;
        ++cellsAlive;
      }
    } else if (fate < pm) {
      grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] =
          grid[cell.x][cell.y][cell.z];
      grid[cell.x][cell.y][cell.z] = false;
      tempCells.emplace_back(Cell(newCellPosition[0], newCellPosition[1],
                                  newCellPosition[2], false, cell.ro));
    } else {
      tempCells.emplace_back(cell);
    }
  } else {
    tempCells.emplace_back(cell);
  }
}

/**
 * @brief Actualiza el estado actual del automáta celular
 *
 * La función itera por cada célula actual del autómata celular escogiendo una célula al azar del vector
 * y la procesa según su tipo
 */
void updateState() {
  while(!cells.empty()){
    unsigned index = rand() % cells.size();
    std::swap(cells[index],cells.back());
    Cell cell = cells.back();
    cells.pop_back();
    if (cell.isStemCell) {
      updateStemCells(cell);
    } else {
      updateNonStemCells(cell);
    }
  }
}

/**
 * @brief Ejecuta la simulación del crecimiento tumoral.
 *
 * La función itera durante el número total de pasos de simulación, actualizando
 * el estado de las células en cada paso. Al finalizar cada día,
 * muestra por consola el día simulado, el número de células vivas
 * y el tiempo transcurrido desde el inicio de la simulación.
 *
 * Tras cada actualización, intercambia el estado actual con el
 * temporal y limpia el contenedor temporal para el siguiente paso.
 */
void simulate() {
  for (int step = 1; step <= nSteps; ++step) {
    updateState();
    if (step % 24 == 0) {
      std::chrono::time_point<std::chrono::high_resolution_clock> elapsedTime;
      elapsedTime = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsedTimeDuration = elapsedTime-start;
      std::cout << "Day: " << step / 24 << "\n";
      std::cout << "cellsAlive: " << cellsAlive << "\n";
      std::cout << "Elapsed time: " << elapsedTimeDuration.count() << "s\n";
    }
    cells.swap(tempCells);
    tempCells.clear();
  }
}

/**
 * @brief Guarda los datos de la simulación en un archivo de texto.
 *
 * Abre el archivo de resultados en modo de anexado y escribe el número
 * de células vivas y el tiempo de ejecución, separados por dos puntos.
 * El nombre del archivo se genera a partir del tamaño de la matriz
 * y del número de días de simulación.
 *
 * Si el archivo no puede abrirse, muestra un mensaje de error por consola.
 *
 * @note Cada registro se añade en una línea nueva, conservando los datos
 *       almacenados previamente en el archivo.
 */
void appendSimulationData() {
  std::ofstream file("simdata1threads" + std::to_string(gridSize) + "grid" +
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
 * @brief Inicializa y ejecuta la simulación del crecimiento tumoral.
 *
 * Inicializa una célula madre en el centro de la cuadrícula tridimensional
 * y establece los contadores iniciales de células vivas. A continuación,
 * registra el instante de inicio, ejecuta la simulación y calcula el tiempo
 * total de ejecución.
 *
 * Finalmente, muestra por consola el tiempo total empleado y guarda los
 * resultados de la simulación en un archivo mediante appendSimulationData().
 *
 * @return 0 si la ejecución finaliza correctamente.
 */
int main() {
  grid[gridSize / 2][gridSize / 2][gridSize / 2] = true;
  cells.emplace_back(gridSize / 2, gridSize / 2, gridSize / 2, true, rhomax);
  stemcellsAlive = 1;
  cellsAlive = 1;
  start = std::chrono::high_resolution_clock::now();
  simulate();
  end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> runTimeDuration = end-start;
  runTime=runTimeDuration.count();
  std::cout << "Total run time: " << runTime << "s\n";
  appendSimulationData();
  return 0;
}