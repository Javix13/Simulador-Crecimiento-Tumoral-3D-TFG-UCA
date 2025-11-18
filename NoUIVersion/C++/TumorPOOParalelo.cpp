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
const int nDays = 80;
const int nSteps = 24 * nDays;
const int min_cells = 0, max_cells = 0;
const int gridSize = 1000;
// crear pools de threads
std::vector<std::thread> threads;
std::vector<std::array<std::array<int, 3>, 2>> threadCoorsInDangerZone;

std::vector<std::unique_ptr<std::mutex>> threadDangerZoneMutexes;
std::vector<std::unique_ptr<std::mutex>> threadCellsMutexes;

// crea un vector para guardar los timestamps por dia

int cellSize = gridSize / N_THREADS;
bool grid[gridSize][gridSize][gridSize] = {{{false}}};
std::vector<std::vector<Cell>> threadCells;
std::vector<std::vector<Cell>> threadTempCells;
double ElapsedTime = 0;

std::atomic<int> stemcellsAlive = 0;
std::atomic<int> nonStemcellsAlive = 0;
std::atomic<int> cellsAlive = 0;
std::map<int, int> emptyNeighboursHist;

// Parámetros de simulación
const double pd = 1.0 / 24.0 * 1.30;
const double pm = 10.0 / 24.0;
const double ps = 0.1;
const int rhomax = 10;
const double alpha = 0.01;

std::barrier<> *barrier;

void countCellsInGrid() {
  int count = 0;
  for (int i = 0; i < gridSize; ++i) {
    for (int j = 0; j < gridSize; ++j) {
      for (int k = 0; k < gridSize; ++k) {
        if (grid[i][j][k]) {
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

void leaveDangerZone(int thread_id, int mutexID) {
  if (mutexID != -1) {
    threadDangerZoneMutexes[mutexID]->lock();
    threadCoorsInDangerZone[mutexID][mutexID == thread_id ? 0 : 1] = {-1, -1,
                                                                      -1};
    threadDangerZoneMutexes[mutexID]->unlock();
  }
}

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
    auto newCellPosition = neighbors[rand() % neighbors.size()];
    double fate = distribution(generator);
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
    auto newCellPosition = neighbors[rand() % neighbors.size()];

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

void updateState(int thread_id, int start_index, int end_index,
                 std::default_random_engine &generator,
                 std::uniform_real_distribution<double> &distribution) {
  while (!threadCells[thread_id].empty()) {

    int index = rand() % threadCells[thread_id].size();
    Cell cell = threadCells[thread_id][index];
    threadCells[thread_id].erase(threadCells[thread_id].begin() + index);
    if (cell.isStemCell) {
      updateStemCells(thread_id, start_index, end_index, cell, generator,
                      distribution);
    } else {
      updateNonStemCells(thread_id, start_index, end_index, cell, generator,
                         distribution);
    }
  }
}

void parallel_simulation(int thread_id, int start_index, int end_index) {
  std::default_random_engine generator(time(0));
  std::uniform_real_distribution<double> distribution(0.0, 1.0);
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds;
  for (int step = 1; step <= nSteps; ++step) {
    updateState(thread_id, start_index, end_index, generator, distribution);
    barrier->arrive_and_wait();
    if (thread_id == 0 && step % 24 == 0) {
      end = std::chrono::system_clock::now();
      elapsed_seconds = end - start;
      auto elapsed = elapsed_seconds.count();

      std::cout << "Day: " << step / 24 << "\n";
      std::cout << "cellsAlive: " << cellsAlive << "\n";
      std::cout << "Elapsed time: " << elapsed << "s\n";
      ElapsedTime += elapsed;
      start = std::chrono::system_clock::now();
    }
    threadCells[thread_id].clear();
    threadCells[thread_id] = threadTempCells[thread_id];
    threadTempCells[thread_id].clear();
    barrier->arrive_and_wait();
  }
}

void verifyCellCount() {
  int totalCells = 0;
  int thread = 0;
  for (const auto &cells : threadCells) {
    std::cout << "Cells in thread " << thread << ":" + cells.size()
              << std::endl;
    totalCells += cells.size();
  }
  if (totalCells == cellsAlive) {
    std::cout << "The cell count matches: " << totalCells << "\n";
  } else {
    std::cout << "Mismatch in cell count. Total cells: " << totalCells
              << ", cellsAlive: " << cellsAlive << "\n";
  }
}

void appendSimulationData() {
  std::ofstream file("simdata" + std::to_string(N_THREADS) + "threads" +
                         std::to_string(gridSize) + "grid" +
                         std::to_string(nDays) + "days.txt",
                     std::ios::app);
  if (file.is_open()) {
    file << cellsAlive << ":" << ElapsedTime << "\n";
    file.close();
  } else {
    std::cerr << "Error opening file for appending data.\n";
  }
}

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
  std::cout << "stemcellsAlive: " << stemcellsAlive << "\n";
  std::cout << "nonStemcellsAlive: " << nonStemcellsAlive << "\n";
  std::cout << "cellsAlive: " << cellsAlive << "\n";
  std::cout << "Elapsed time: " << ElapsedTime << "s\n";
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
