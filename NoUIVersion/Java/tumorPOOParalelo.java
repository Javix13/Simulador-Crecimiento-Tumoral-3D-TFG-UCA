import java.io.FileWriter;
import java.io.IOException;
import java.time.Duration;
import java.time.Instant;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantLock;

class Cell {
  int x, y, z;
  boolean isStemCell;
  int ro;

  Cell(int x, int y, int z, boolean isStemCell, int ro) {
    this.x = x;
    this.y = y;
    this.z = z;
    this.isStemCell = isStemCell;
    this.ro = ro;
  }
}
public class tumorPOOParalelo implements Runnable {
  int thread_id;
  int start_index;
  int end_index;
  tumorPOOParalelo(int thread_id, int start_index, int end_index) {
    this.thread_id = thread_id;
    this.start_index = start_index;
    this.end_index = end_index;
  }

  static int N_THREADS;
  static final int nDays = 80;
  static final int nSteps = 24 * nDays;
  static final int gridSize = 1000;
  static final double pd = 1.0 / 24.0 * 1.30;
  static final double pm = 10.0 / 24.0;
  static final double ps = 0.1;
  static final int rhomax = 10;
  static final double alpha = 0.01;
  static Instant startTime, endTime;

  static boolean[][][] grid = new boolean[gridSize][gridSize][gridSize];
  static int cellSize;
  static List<Thread> threads = new ArrayList<>();
  static List<List<Cell>> threadCells = new ArrayList<>();
  static List<List<Cell>> threadTempCells = new ArrayList<>();
  static List<ReentrantLock> threadCellsLocks = new ArrayList<>();
  static List<ReentrantLock> dangerZoneLocks = new ArrayList<>();
  static List<int[][]> threadCoorsInDangerZone =
      new ArrayList<>(); // Each: { {incoming}, {outgoing} }

  static AtomicInteger stemcellsAlive = new AtomicInteger(0);
  static AtomicInteger nonStemcellsAlive = new AtomicInteger(0);
  static AtomicInteger cellsAlive = new AtomicInteger(0);

  static CyclicBarrier barrier;
  static double ElapsedTime = 0;

  static boolean inDangerZone(int x, int y, int z, int start_index,
                              int end_index, int thread_id, int[] mutexID) {
    if (thread_id != 0 && thread_id != N_THREADS - 1) {
      if (x <= start_index + 1) {
        mutexID[0] = thread_id - 1;
        dangerZoneLocks.get(mutexID[0]).lock();
        int[] threadCellcoor = threadCoorsInDangerZone.get(mutexID[0])[0];
        if (threadCellcoor[0] != -1 & Math.abs(threadCellcoor[0] - x) <= 2 &&
            Math.abs(threadCellcoor[1] - y) <= 2 &&
            Math.abs(threadCellcoor[2] - z) <= 2) {
          dangerZoneLocks.get(mutexID[0]).unlock();
          return true;
        } else {
          threadCoorsInDangerZone.get(mutexID[0])[1] = new int[] {x, y, z};
          dangerZoneLocks.get(mutexID[0]).unlock();
          return false;
        }
      }
      if (x >= end_index - 1) {
        mutexID[0] = thread_id;
        dangerZoneLocks.get(mutexID[0]).lock();
        int[] threadCellcoor = threadCoorsInDangerZone.get(mutexID[0])[1];
        if (threadCellcoor[0] != -1 & Math.abs(threadCellcoor[0] - x) <= 2 &&
            Math.abs(threadCellcoor[1] - y) <= 2 &&
            Math.abs(threadCellcoor[2] - z) <= 2) {
          dangerZoneLocks.get(mutexID[0]).unlock();
          return true;
        } else {
          threadCoorsInDangerZone.get(mutexID[0])[0] = new int[] {x, y, z};
          dangerZoneLocks.get(mutexID[0]).unlock();
          return false;
        }
      } else {
        return false;
      }

    } else {
      if (thread_id == 0) {
        mutexID[0] = thread_id;
        if (x < end_index - 1) {
          return false;
        }
        dangerZoneLocks.get(mutexID[0]).lock();
        int[] threadCellcoor = threadCoorsInDangerZone.get(mutexID[0])[1];
        if (threadCellcoor[0] != -1 & Math.abs(threadCellcoor[0] - x) <= 2 &&
            Math.abs(threadCellcoor[1] - y) <= 2 &&
            Math.abs(threadCellcoor[2] - z) <= 2) {
          dangerZoneLocks.get(mutexID[0]).unlock();
          return true;
        } else {
          threadCoorsInDangerZone.get(mutexID[0])[0] = new int[] {x, y, z};
          dangerZoneLocks.get(mutexID[0]).unlock();
          return false;
        }
      } else {
        mutexID[0] = thread_id - 1;
        if (x > start_index + 1) {
          return false;
        }
        dangerZoneLocks.get(mutexID[0]).lock();
        int[] threadCellcoor = threadCoorsInDangerZone.get(mutexID[0])[0];
        if (threadCellcoor[0] != -1 & Math.abs(threadCellcoor[0] - x) <= 2 &&
            Math.abs(threadCellcoor[1] - y) <= 2 &&
            Math.abs(threadCellcoor[2] - z) <= 2) {
          dangerZoneLocks.get(mutexID[0]).unlock();
          return true;
        } else {
          threadCoorsInDangerZone.get(mutexID[0])[1] = new int[] {x, y, z};
          dangerZoneLocks.get(mutexID[0]).unlock();
          return false;
        }
      }
    }
  }
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
  static void leaveDangerZone(int thread_id, int[] mutexID) {
    if (mutexID[0] != -1) {
      dangerZoneLocks.get(mutexID[0]).lock();
      threadCoorsInDangerZone.get(mutexID[0])[mutexID[0] == thread_id ? 0 : 1] =
          new int[] {-1, -1, -1};
      dangerZoneLocks.get(mutexID[0]).unlock();
    }
  }
  static void updateStemCells(int thread_id, int start_index, int end_index,
                              Cell cell) {
    int[] mutexID = {-1};
    if (inDangerZone(cell.x, cell.y, cell.z, start_index, end_index, thread_id,
                     mutexID)) {
      threadCells.get(thread_id).add(cell);
      return;
    }
    List<int[]> neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);
    if (!neighbors.isEmpty()) {
      int[] newCellPosition =
          neighbors.get(new Random().nextInt(neighbors.size()));
      double fate = new Random().nextDouble();
      if (fate < pd) {
        fate = new Random().nextDouble();
        if (fate <= ps) {
          grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] =
              true;
          int vectorIndex =
              Math.min(newCellPosition[0] / cellSize, N_THREADS - 1);
          threadCellsLocks.get(vectorIndex).lock();
          threadTempCells.get(vectorIndex)
              .add(new Cell(newCellPosition[0], newCellPosition[1],
                            newCellPosition[2], true, rhomax));
          threadCellsLocks.get(vectorIndex).unlock();
          threadCellsLocks.get(thread_id).lock();
          threadTempCells.get(thread_id).add(
              new Cell(cell.x, cell.y, cell.z, true, cell.ro));
          threadCellsLocks.get(thread_id).unlock();
          stemcellsAlive.getAndIncrement();
          cellsAlive.getAndIncrement();
        } else {
          grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] =
              true;
          int vectorIndex =
              Math.min(newCellPosition[0] / cellSize, N_THREADS - 1);
          threadCellsLocks.get(vectorIndex).lock();
          threadTempCells.get(vectorIndex)
              .add(new Cell(newCellPosition[0], newCellPosition[1],
                            newCellPosition[2], false, cell.ro));
          threadCellsLocks.get(vectorIndex).unlock();
          threadCellsLocks.get(thread_id).lock();
          threadTempCells.get(thread_id).add(
              new Cell(cell.x, cell.y, cell.z, true, cell.ro));
          threadCellsLocks.get(thread_id).unlock();
          nonStemcellsAlive.getAndIncrement();
          cellsAlive.getAndIncrement();
        }
      } else if (fate < pm) {
        int[] newCoor = neighbors.get(new Random().nextInt(neighbors.size()));
        grid[newCoor[0]][newCoor[1]][newCoor[2]] = true;
        grid[cell.x][cell.y][cell.z] = false;
        int vectorIndex = Math.min(newCoor[0] / cellSize, N_THREADS - 1);
        threadCellsLocks.get(vectorIndex).lock();
        threadTempCells.get(vectorIndex)
            .add(new Cell(newCoor[0], newCoor[1], newCoor[2], true, rhomax));
        threadCellsLocks.get(vectorIndex).unlock();
      } else {
        threadCellsLocks.get(thread_id).lock();
        threadTempCells.get(thread_id).add(cell);
        threadCellsLocks.get(thread_id).unlock();
      }
    } else {
      threadCellsLocks.get(thread_id).lock();
      threadTempCells.get(thread_id).add(cell);
      threadCellsLocks.get(thread_id).unlock();
    }
    leaveDangerZone(thread_id, mutexID);
  }

  static void updateNonStemCells(int thread_id, int start_index, int end_index,
                                 Cell cell) {
    int[] mutexID = {-1};
    if (inDangerZone(cell.x, cell.y, cell.z, start_index, end_index, thread_id,
                     mutexID)) {
      threadCells.get(thread_id).add(cell);
      return;
    }
    double fate = new Random().nextDouble();
    if (fate < alpha) {
      grid[cell.x][cell.y][cell.z] = false;
      nonStemcellsAlive.getAndDecrement();
      cellsAlive.getAndDecrement();
      leaveDangerZone(thread_id, mutexID);
      return;
    }

    List<int[]> neighbors = getEmptyNeighbors(cell.x, cell.y, cell.z);
    if (!neighbors.isEmpty()) {
      fate = new Random().nextDouble();
      int[] newCellPosition =
          neighbors.get(new Random().nextInt(neighbors.size()));
      if (fate < pd) {
        if (cell.ro == 0) {
          grid[cell.x][cell.y][cell.z] = false;
          nonStemcellsAlive.getAndDecrement();
          cellsAlive.getAndDecrement();

        } else {
          cell.ro--;
          grid[newCellPosition[0]][newCellPosition[1]][newCellPosition[2]] =
              true;
          int vectorIndex =
              Math.min(newCellPosition[0] / cellSize, N_THREADS - 1);
          threadCellsLocks.get(vectorIndex).lock();
          threadTempCells.get(vectorIndex).add(cell);
          threadCellsLocks.get(vectorIndex).unlock();
          threadCellsLocks.get(thread_id).lock();
          threadTempCells.get(thread_id).add(
              new Cell(newCellPosition[0], newCellPosition[1],
                       newCellPosition[2], false, cell.ro));
          threadCellsLocks.get(thread_id).unlock();
          nonStemcellsAlive.incrementAndGet();
          cellsAlive.incrementAndGet();
        }
      } else if (fate < pm) {
        int[] newCoor = neighbors.get(new Random().nextInt(neighbors.size()));
        grid[newCoor[0]][newCoor[1]][newCoor[2]] = true;
        grid[cell.x][cell.y][cell.z] = false;
        int vectorIndex = Math.min(newCoor[0] / cellSize, N_THREADS - 1);
        threadCellsLocks.get(vectorIndex).lock();
        threadTempCells.get(vectorIndex)
            .add(new Cell(newCoor[0], newCoor[1], newCoor[2], false, cell.ro));
        threadCellsLocks.get(vectorIndex).unlock();
      } else {
        threadCellsLocks.get(thread_id).lock();
        threadTempCells.get(thread_id).add(cell);
        threadCellsLocks.get(thread_id).unlock();
      }
    } else {
      threadCellsLocks.get(thread_id).lock();
      threadTempCells.get(thread_id).add(cell);
      threadCellsLocks.get(thread_id).unlock();
    }
    leaveDangerZone(thread_id, mutexID);
  }

  static void updateState(int thread_id, int start_index, int end_index) {
    List<Cell> cells = threadCells.get(thread_id);
    while (!cells.isEmpty()) {
      int index = new Random().nextInt(cells.size());
      Cell cell = cells.remove(index);
      if (cell.isStemCell) {
        updateStemCells(thread_id, start_index, end_index, cell);
      } else {
        updateNonStemCells(thread_id, start_index, end_index, cell);
      }
    }
  }
  static void parallel_simulation(int thread_id, int start_index,
                                  int end_index) {
    if (thread_id == 0) {
      startTime = Instant.now();
    }
    for (int step = 1; step <= nSteps; step++) {
      updateState(thread_id, start_index, end_index);
      try {
        barrier.await();
      } catch (InterruptedException | BrokenBarrierException e) {
        e.printStackTrace();
      }
      if (thread_id == 0 && step % 24 == 0) {
        endTime = Instant.now();
        ElapsedTime += Duration.between(startTime, endTime).toMillis() / 1000;
        System.out.println("Day: " + (step / 24));
        System.out.println("cellsAlive: " + cellsAlive);
        System.out.println("stemcellsAlive: " + stemcellsAlive);
        System.out.println("nonStemcellsAlive: " + nonStemcellsAlive);
        startTime = Instant.now();
      }
      threadCells.get(thread_id).clear();
      threadCells.set(thread_id,
                      new ArrayList<>(threadTempCells.get(thread_id)));
      threadTempCells.get(thread_id).clear();
      try {
        barrier.await();
      } catch (InterruptedException | BrokenBarrierException e) {
        e.printStackTrace();
      }
    }
  }
  @Override
  public void run() {
    parallel_simulation(thread_id, start_index, end_index);
  }
  static void appendSimulationData() {
    String filename = "simdata" + N_THREADS + "threads" + gridSize + "grid" +
                      nDays + "days.txt";
    try (FileWriter file = new FileWriter(filename, true)) {
      file.write(cellsAlive + ":" + ElapsedTime + "\n");
    } catch (IOException e) {
      System.err.println("Error opening file for appending data.");
    }
  }

  static void verifyCellCount() {
    int totalCells = 0;
    for (List<Cell> cellList : threadCells) {
      totalCells += cellList.size();
    }
    if (totalCells != cellsAlive.get()) {
      System.err.println("Cell count mismatch: expected " + cellsAlive +
                         ", found " + totalCells);
    } else {
      System.out.println("Cell count verified: " + totalCells);
    }
  }
  public static void main(String[] args) {
    if (args.length > 0) {
      N_THREADS = Integer.parseInt(args[0]);
    }
    cellSize = gridSize / N_THREADS;
    barrier = new CyclicBarrier(N_THREADS);

    // Inicializar las listas para cada thread
    for (int i = 0; i < N_THREADS; i++) {
      threadCells.add(new ArrayList<>());
      threadTempCells.add(new ArrayList<>());
      threadCellsLocks.add(new ReentrantLock());
      dangerZoneLocks.add(new ReentrantLock());
      threadCoorsInDangerZone.add(new int[][] {{-1, -1, -1}, {-1, -1, -1}});
    }
    grid[gridSize / 2][gridSize / 2][gridSize / 2] = true;
    threadCells.get(N_THREADS / 2)
        .add(new Cell(gridSize / 2, gridSize / 2, gridSize / 2, true, rhomax));
    stemcellsAlive.set(1);
    cellsAlive.set(1);
    // Crear y arrancar los threads
    for (int i = 0; i < N_THREADS; i++) {
      if (i == N_THREADS - 1) {
        Thread thread = new Thread(
            new tumorPOOParalelo(i, i * (gridSize / N_THREADS), gridSize - 1));
        threads.add(thread);
        thread.start();
      } else {
        Thread thread = new Thread(
            new tumorPOOParalelo(i, i * (gridSize / N_THREADS),
                                 (i + 1) * (gridSize / N_THREADS) - 1));
        threads.add(thread);
        thread.start();
      }
    }

    // Esperar a que todos los threads terminen
    for (Thread thread : threads) {
      try {
        thread.join();
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
    }
    appendSimulationData();
    System.out.println("Simulation completed.");
    System.out.println("Total cells alive: " + cellsAlive);
    System.out.println("Total stem cells alive: " + stemcellsAlive);
    System.out.println("Total non-stem cells alive: " + nonStemcellsAlive);

    System.out.println("Total elapsed time: " + ElapsedTime + " ms");
    // verifyCellCount();
  }
}
