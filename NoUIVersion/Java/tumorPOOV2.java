import java.io.FileWriter;
import java.io.IOException;
import java.time.Duration;
import java.time.Instant;
import java.util.*;

public class tumorPOOV2 {
  // Configuración
  static final int nDays = 120;
  static final int nSteps = 24 * nDays;
  static final int min_cells = 0, max_cells = 0;
  static final int gridSize = 500;
  static boolean[][][] grid = new boolean[gridSize][gridSize][gridSize];
  static ArrayList<Cell> cells = new ArrayList<>();
  static ArrayList<Cell> tempCells = new ArrayList<>();
  //static ArrayList<Integer> cellsAliveByDay = new ArrayList<>();
  static int stemcellsAlive = 0;
  static int nonStemcellsAlive = 0;
  static int cellsAlive = 0;
  static double runTime = 0.0;
  static Instant startTime, endTime;

  // Parámetros de simulación
  static final double pd = 1.0 / 24.0 * 1.30;
  static final double pm = 10.0 / 24.0;
  static final double ps = 0.1;
  static final int rhomax = 10;
  static final double alpha = 0.01;

  static final Random random = new Random();

  static List<int[]> getEmptyNeighbors(int i, int j, int k) {
    List<int[]> neighbors = new ArrayList<>();
    for (int x = -1; x <= 1; ++x) {
      for (int y = -1; y <= 1; ++y) {
        for (int z = -1; z <= 1; ++z) {
          if (x == 0 && y == 0 && z == 0)
            continue;
          int ni = i + x, nj = j + y, nk = k + z;
          if (ni >= 0 && ni < gridSize && nj >= 0 && nj < gridSize && nk >= 0 &&
              nk < gridSize && !grid[ni][nj][nk]) {
            neighbors.add(new int[] {ni, nj, nk});
          }
        }
      }
    }
    return neighbors;
  }

  static void updateStemCells(Cell cell) {
    List<int[]> neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);
    if (!neighbors.isEmpty()) {
      double fate = random.nextDouble();
      int[] newCellPosition = neighbors.get(random.nextInt(neighbors.size()));
      if (fate < pd) {
        fate = random.nextDouble();
        grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] = true;
        if (fate <= ps) {
          // División simétrica
          tempCells.add(new Cell(newCellPosition[0], newCellPosition[1],
                                 newCellPosition[2], true, rhomax));
          tempCells.add(new Cell(cell.x, cell.y, cell.z, true, cell.ro));
          stemcellsAlive++;
          cellsAlive++;
        } else {
          // División asimétrica
          tempCells.add(new Cell(newCellPosition[0], newCellPosition[1],
                                 newCellPosition[2], false, rhomax));
          tempCells.add(new Cell(cell.x, cell.y, cell.z, true, cell.ro));
          nonStemcellsAlive++;
          cellsAlive++;
        }
      } else if (fate < pm) {
        int[] newCoor = neighbors.get(random.nextInt(neighbors.size()));
        grid[newCoor[0]][newCoor[1]][newCoor[2]] = true;
        grid[cell.x][cell.y][cell.z] = false;
        tempCells.add(
            new Cell(newCoor[0], newCoor[1], newCoor[2], true, rhomax));
      } else {
        tempCells.add(cell);
      }
    } else {
      tempCells.add(cell);
    }
  }

  static void updateNonStemCells(Cell cell) {
    double fate = random.nextDouble();
    if (fate < alpha) {
      grid[cell.x][cell.y][cell.z] = false;
      nonStemcellsAlive--;
      cellsAlive--;
      return;
    }

    List<int[]> neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);
    if (!neighbors.isEmpty()) {
      fate = random.nextDouble();
      int[] newCellPosition = neighbors.get(random.nextInt(neighbors.size()));
      if (fate < pd) {
        if (cell.ro == 0) {
          grid[cell.x][cell.y][cell.z] = false;
          nonStemcellsAlive--;
          cellsAlive--;
        } else {
          cell.ro--;
          grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] =
              true;
          tempCells.add(cell);
          tempCells.add(new Cell(newCellPosition[0], newCellPosition[1],
                                 newCellPosition[2], false, cell.ro));
          nonStemcellsAlive++;
          cellsAlive++;
        }
      } else if (fate < pm) {
        int[] newCoor = neighbors.get(random.nextInt(neighbors.size()));
        grid[newCoor[0]][newCoor[1]][newCoor[2]] = true;
        grid[cell.x][cell.y][cell.z] = false;
        tempCells.add(
            new Cell(newCoor[0], newCoor[1], newCoor[2], false, cell.ro));
      } else {
        tempCells.add(cell);
      }
    } else {
      tempCells.add(cell);
    }
  }

  static void updateState() {
    while (!cells.isEmpty()) {
      int index = random.nextInt(cells.size());
      Collections.swap(cells, index, cells.size()-1);
      Cell cell = cells.removeLast();
      if (cell.isStemCell) {
        updateStemCells(cell);
      } else {
        updateNonStemCells(cell);
      }
    }
  }

  static void simulate() {
    startTime= Instant.now();
    for (int step = 1; step <= nSteps; step++) {
      updateState();
      if (step % 24 == 0) {
        Instant elapsedTime= Instant.now();
        //cellsAliveByDay.add(cellsAlive);
        System.out.println("Day: " + (step / 24));
        System.out.println("cellsAlive: " + cellsAlive);
        System.out.println("Elapsed time: " + Duration.between(startTime, elapsedTime).toMillis() / 1000 + "s");
      }
      cells = new ArrayList<>(tempCells);
      tempCells.clear();
    }
    endTime=Instant.now();
    runTime = Duration.between(startTime, endTime).toMillis() / 1000;
    System.out.println("Total run time: " + runTime + "s");
  }

  static void appendSimulationData() {
    String filename =
        "simdata1threads" + gridSize + "grid" + nDays + "days.txt";
    try (FileWriter file = new FileWriter(filename, true)) {
      file.write(cellsAlive + ":" + runTime + "\n");
    } catch (IOException e) {
      System.err.println("Error opening file for appending data.");
    }
  }

  public static void main(String[] args) {
    System.out.println("Simulation in Java\n");
    int mid = gridSize / 2;
    grid[mid][mid][mid] = true;
    cells.add(new Cell(mid, mid, mid, true, rhomax));
    stemcellsAlive = 1;
    cellsAlive = 1;
    simulate();

    appendSimulationData();
  }
}
