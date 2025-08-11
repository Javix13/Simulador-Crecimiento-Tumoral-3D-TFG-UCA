#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <array>
#include <random>
#include <ctime>
#include <map>
#include <chrono>

// Clase para representar una célula
struct Cell {
    int x, y;
    bool isStemCell;
    int ro;

    Cell(int x, int y, bool isStemCell, int ro)
        : x(x), y(y), isStemCell(isStemCell), ro(ro) {}
};

// Variables globales
const int nDays = 50;
const int nSteps = 24 * nDays;
const int min_cells = 0,max_cells=0;
const int gridSize = 750;
bool grid[gridSize][gridSize]= {{{false}}};
std::vector<Cell> cells;
std::vector<Cell> tempCells;
std::vector<int> cellsAliveByDay;
int stemcellsAlive = 0;
int nonStemcellsAlive = 0;
int cellsAlive = 0;
double ElapsedTime = 0;

// Parámetros de simulación
const double pd = 1.0 / 24.0;
const double pm = 10.0 / 24.0;
const double ps = 0.1;
const int rhomax = 10;
const double alpha = 0.01;
std::default_random_engine generator(std::time(0));
std::uniform_real_distribution<double> distribution(0.0, 1.0);


// Función para obtener vecinos vacíos
std::vector<std::array<int, 2>> getEmptyNeighbors(int i, int j) {
    std::vector<std::array<int, 2>> neighbors;

    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
                if (x == 0 && y == 0) continue;
                int ni = i + x, nj = j + y;
                if (ni >= 0 && ni < gridSize && nj >= 0 && nj < gridSize && !grid[ni][nj]) {
                    neighbors.push_back({ni, nj});
                }
        }
    }
    return neighbors;
}

// Función para actualizar células madre
void updateStemCells(Cell cell) {
    auto neighbors = getEmptyNeighbors(cell.x, cell.y);

    if (!neighbors.empty()) {
        double fate = distribution(generator);
        if (fate < pd) {
            fate = distribution(generator);
            auto position = neighbors[rand() % neighbors.size()];

            if (fate <= ps) {//division simetrica
                grid[position[0]][position[1]] = true;
                tempCells.emplace_back(Cell(position[0], position[1], true, rhomax));
                tempCells.emplace_back(Cell(cell.x, cell.y, true, cell.ro));
                ++stemcellsAlive;
                ++cellsAlive;
            } else { //division asimetrica
                grid[position[0]][position[1]] = true;
                tempCells.emplace_back(Cell(position[0],position[1], false, rhomax));
                tempCells.emplace_back(Cell(cell.x, cell.y, true, cell.ro));
                ++nonStemcellsAlive;
                ++cellsAlive;
            }
        } else if (fate < pm) { //migracion
            auto newCoor = neighbors[rand() % neighbors.size()];
            grid[newCoor[0]][newCoor[1]] = grid[cell.x][cell.y];
            grid[cell.x][cell.y]= false;
            tempCells.emplace_back(Cell(newCoor[0], newCoor[1], true, rhomax));
        } else { //nada
            tempCells.emplace_back(cell);
        }
    } else { //nada
        tempCells.emplace_back(cell);
    }
}

// Función para actualizar células no madre
void updateNonStemCells(Cell cell) {
    double fate = distribution(generator);
    if (fate < alpha) {
        grid[cell.x][cell.y]= false;
        --nonStemcellsAlive;
        --cellsAlive;
        return;
    }

    auto neighbors = getEmptyNeighbors(cell.x, cell.y);

    if (!neighbors.empty()) {
        fate = distribution(generator);

        if (fate < pd && cell.ro > 0) {
            --cell.ro;
            auto newCellPosition = neighbors[rand() % neighbors.size()];
            grid[newCellPosition[0]][newCellPosition[1]] = true;
            tempCells.emplace_back(cell);
            tempCells.emplace_back(Cell(newCellPosition[0], newCellPosition[1], false, cell.ro));
            ++nonStemcellsAlive;
            ++cellsAlive;
        } else if (cell.ro == 0) {
            grid[cell.x][cell.y] = false;
            --nonStemcellsAlive;
            --cellsAlive;
        } else if (fate < pm) {
            auto newCoor = neighbors[rand() % neighbors.size()];
            grid[newCoor[0]][newCoor[1]] = grid[cell.x][cell.y];
            grid[cell.x][cell.y] = false;
            tempCells.emplace_back(Cell(newCoor[0], newCoor[1], false, cell.ro));
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
            if(step % 24 == 0){
                end = std::chrono::system_clock::now();
                elapsed_seconds = end - start;
                double elapsed= elapsed_seconds.count();
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
    std::ofstream file("simdata1threads"+std::to_string(gridSize)+"grid"+std::to_string(nDays)+"days.txt", std::ios::app);
    if (file.is_open()) {
        file << cellsAlive << ":" << ElapsedTime << "\n";
        file.close();
    } else {
        std::cerr << "Error opening file for appending data.\n";
    }
}


// void countCellsInGrid() {
//     int count = 0;
//     for (int i = 0; i < gridSize; ++i) {
//         for (int j = 0; j < gridSize; ++j) {
//                 if (grid[i][j]) {
//                     ++count;
//                 }
//         }
//     }
//     std::cout << "Total cells in grid: " << count << "\n";
//     if (count == cellsAlive) {
//         std::cout << "The number of cells in the grid matches cellsAlive: " << count << "\n";
//     } else {
//         std::cout << "Mismatch! Number of cells in the grid: " << count << ", cellsAlive: " << cellsAlive << "\n";
//     }
// }
int main() {
    grid[gridSize / 2][gridSize / 2] = true;
    cells.emplace_back(gridSize / 2, gridSize / 2, true, rhomax);
    stemcellsAlive = 1;
    cellsAlive = 1;
    simulate();
    if((min_cells!=0 && cellsAlive<min_cells) || (max_cells!=0 && cellsAlive>max_cells)){
        std::cout<<"Simulation discarded, cells alive: "<<cellsAlive<<std::endl;
        return 0;
    }
    countCellsInGrid();
    appendSimulationData();
    return 0;
}
