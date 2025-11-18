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
const int nDays = 80;
const int nSteps = 24 * nDays;
const int gridSize = 1000;
bool grid[gridSize][gridSize][gridSize] = {{{false}}};
std::vector<Cell> cells;
std::vector<Cell> tempCells;
std::vector<int> cellsAliveByDay;
int stemcellsAlive = 0;
int nonStemcellsAlive = 0;
int cellsAlive = 0;
double ElapsedTime = 0;

// Parámetros de simulación
const double pd = 1.0 / 24.0 * 1.30;
const double pm = 10.0 / 24.0;
const double ps = 0.1;
const int rhomax = 10;
const double alpha = 0.01;
std::default_random_engine generator(std::time(0));
std::uniform_real_distribution<double> distribution(0.0, 1.0);

// Función para obtener vecinos vacíos
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

// Función para actualizar células madre
void updateStemCells(Cell cell) {
  auto neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);

  if (!neighbors.empty()) {
    double fate = distribution(generator);
    auto newCellPosition = neighbors[rand() % neighbors.size()];
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

// Función para actualizar células no madre
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
    auto newCellPosition = neighbors[rand() % neighbors.size()];
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

// Función para actualizar el estado general
void updateState() {

  while (!cells.empty()) {
    int index = rand() % cells.size();
    Cell cell = cells[index];
    cells.erase(cells.begin() + index);

    if (cell.isStemCell) {
      updateStemCells(cell);
    } else {
      updateNonStemCells(cell);
    }
  }
}

// Función para simular
void simulate() {
  std::chrono::time_point<std::chrono::system_clock> start, end;
  std::chrono::duration<double> elapsed_seconds;
  start = std::chrono::system_clock::now();

  for (int step = 1; step <= nSteps; ++step) {
    updateState();
    if (step % 24 == 0) {
      end = std::chrono::system_clock::now();
      elapsed_seconds = end - start;
      double elapsed = elapsed_seconds.count();
      cellsAliveByDay.push_back(cellsAlive);
      ElapsedTime += elapsed;
      start = std::chrono::system_clock::now();
      std::cout << "Day: " << step / 24 << "\n";
      std::cout << "cellsAlive: " << cellsAlive << "\n";
      std::cout << "Elapsed time: " << elapsed << "s\n";
      start = std::chrono::system_clock::now();
    }
    cells.clear();
    cells = tempCells;
    tempCells.clear();
  }
  std::cout << "Total elapsed time: " << ElapsedTime << "s\n";
}
void appendSimulationData() {
  std::ofstream file("simdata1threads" + std::to_string(gridSize) + "grid" +
                         std::to_string(nDays) + "days.txt",
                     std::ios::app);
  if (file.is_open()) {
    file << cellsAlive << ":" << ElapsedTime << "\n";
    file.close();
  } else {
    std::cerr << "Error opening file for appending data.\n";
  }
}

int main() {
  grid[gridSize / 2][gridSize / 2][gridSize / 2] = true;
  cells.emplace_back(gridSize / 2, gridSize / 2, gridSize / 2, true, rhomax);
  stemcellsAlive = 1;
  cellsAlive = 1;
  simulate();
  appendSimulationData();
  return 0;
}
