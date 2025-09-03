using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.UI;

public class MasterScriptv02 : MonoBehaviour
{
    // Evento para notificar cambio de día
    public event System.Action<int, int, int, int> OnDayChanged;
    public Dictionary<string, float> sc_traitvector = new Dictionary<string, float> {
        { "cct", 24 }, { "rho_max", 10 }, { "mu", 100 }, { "alpha", 0 }
    };
    public Dictionary<string, float> nsc_traitvector = new Dictionary<string, float> {
        { "cct", 24 }, { "rho_max", 10 }, { "mu", 100 }, { "alpha", 0.01f }
    };

    private float inct;
    private float pd;
    private float pm;
    private readonly float updateInterval = 0.2f;
    public float ps = 0.1f;
    public int grid_size;
    public GameObject scellPrefab;
    public GameObject nscellPrefab;
    private List<GameObject> PreInstantiatedCells;
    public int CellsAlive;
    public int StemCellsAlive;
    public int NonStemCellsAlive;
    private int step = 1;

    public int days = 80;
    public int nSteps;
    public Text text;
    public float time;
    private bool isSimulating = false;
    public Stopwatch stopwatch1 = new Stopwatch();
    public Stopwatch stopwatch2 = new Stopwatch();
    public Stopwatch stopwatch3 = new Stopwatch();


    bool[,,] grid;
    List<Cell> cells;
    List<Cell> tempCells;
    private void Awake()
    {

        nSteps = days * 24;
        grid = new bool[grid_size, grid_size, grid_size];
        inct = 1.30f;
        pd = 1.0f / 24.0f * inct;
        pm = 10.0f / 24.0f;
        cells = new List<Cell>();
        tempCells = new List<Cell>();
        PreInstantiatedCells = new List<GameObject>();
        int center = grid_size / 2;
        grid[center, center, center] = true;
        cells.Add(new Cell(center, center, center, true, (int)sc_traitvector["rho_max"]));
        CellsAlive = 1;
        StemCellsAlive = 1;
        NonStemCellsAlive = 0;
        //UnityEngine.Random.InitState(0);
        // get from dont destroy on load object the simulation parameters
        SimulationParametersGO simulationParametersGO = FindAnyObjectByType<SimulationParametersGO>();
        if (simulationParametersGO != null)
        {
            UnityEngine.Debug.Log("Simulation Parameters Found");
            days = simulationParametersGO.nDays;
            ps = simulationParametersGO.ps;
            sc_traitvector["rho"] = simulationParametersGO.rho;
            nsc_traitvector["rho"] = simulationParametersGO.rho;
            sc_traitvector["alpha"] = simulationParametersGO.alpha;
            nsc_traitvector["alpha"] = simulationParametersGO.alpha;
            nSteps = days * 24;
            UnityEngine.Debug.Log("Days: " + days + " ps: " + ps + " rho: " + sc_traitvector["rho"] + " alpha: " + sc_traitvector["alpha"]);

        }
    }
    private void Start()
    {
        time = Time.realtimeSinceStartup;
        for (int i = 1; i < 300000; i++)
        {
            GameObject cell = Instantiate(scellPrefab, new Vector3(0, 0, 0), Quaternion.identity);
            cell.SetActive(false);
            PreInstantiatedCells.Add(cell);
        }
        isSimulating = true;
        StartCoroutine(Simulate());
    }

    private void Update()
    {
        // si el usuario pulsa la espacio, inicia o para la simulacion;
        if (Keyboard.current.spaceKey.wasPressedThisFrame)
        {
            isSimulating = !isSimulating;
            if (isSimulating)
            {
                StartCoroutine(Simulate());
            }
        }
    }

    private IEnumerator Simulate()
    {
        int chunkSize = 1000; // Número de células a procesar por frame
        while (isSimulating && step <= nSteps)
        {
            // Procesar células en bloques
            int totalCells = cells.Count;
            for (int i = 0; i < totalCells; i += chunkSize)
            {
                int end = Mathf.Min(i + chunkSize, totalCells);
                for (int j = i; j < end; j++)
                {
                    Cell cell = cells[j];
                    if (cell.isStemCell)
                        Update_scells(cell);
                    else
                        Update_nscells(cell);
                }
                yield return null; // Espera al siguiente frame antes de continuar con el siguiente bloque
            }
            // Copiar tempCells a cells y limpiar tempCells
            cells = new List<Cell>(tempCells);
            tempCells.Clear();
            Instantiate_cells();
            yield return new WaitForSeconds(updateInterval);

            if (step % 24 == 0)
            {
                int day = step / 24;
                OnDayChanged?.Invoke(day, CellsAlive, StemCellsAlive, NonStemCellsAlive);
            }
            step++;
        }

    }
    // UpdateState ya no es necesario, la lógica se movió a Simulate con procesamiento por bloques

    private void Update_scells(Cell cell)
    {

        List<Vector3Int> neighbors = GetEmptyNeighbors(cell.x, cell.y, cell.z);
        if (neighbors.Count > 0)
        {
            float fate = UnityEngine.Random.Range(0.0f, 1.0f);
            Vector3Int position = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
            if (fate < pd)
            {
                fate = UnityEngine.Random.Range(0.0f, 1.0f);

                if (fate <= ps)
                {
                    grid[position.x, position.y, position.z] = true;
                    tempCells.Add(new Cell(position.x, position.y, position.z, true, (int)sc_traitvector["rho_max"]));
                    tempCells.Add(new Cell(cell));
                    StemCellsAlive += 1;
                    CellsAlive += 1;
                }
                else
                {
                    grid[position.x, position.y, position.z] = true;
                    tempCells.Add(new Cell(position.x, position.y, position.z, false, (int)nsc_traitvector["rho_max"]));
                    tempCells.Add(new Cell(cell));
                    NonStemCellsAlive += 1;
                    CellsAlive += 1;
                }

            }
            else
            {
                if (fate < pm)
                {
                    Vector3Int newcoor = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                    grid[newcoor.x, newcoor.y, newcoor.z] = grid[cell.x, cell.y, cell.z];
                    tempCells.Add(new Cell(newcoor.x, newcoor.y, newcoor.z, true, (int)sc_traitvector["rho_max"]));

                }
                else
                {
                    tempCells.Add(new Cell(cell));
                }
            }
        }
        else
        {
            tempCells.Add(new Cell(cell));
        }


    }

    private void Update_nscells(Cell cell)
    {
        float fate = UnityEngine.Random.Range(0.0f, 1.0f);
        if (fate < nsc_traitvector["alpha"])
        {
            grid[cell.x, cell.y, cell.z] = false;
            NonStemCellsAlive -= 1;
            CellsAlive -= 1;
            return;
        }
        List<Vector3Int> neighbors = GetEmptyNeighbors(cell.x, cell.y, cell.z);
        if (neighbors.Count > 0)
        {
            fate = UnityEngine.Random.Range(0.0f, 1.0f);
            Vector3Int newcell_position = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
            if (fate < pd)
            {
                if (cell.ro == 0)
                {
                    grid[cell.x, cell.y, cell.z] = false;
                    NonStemCellsAlive -= 1;
                    CellsAlive -= 1;
                }
                else
                {
                    cell.ro -= 1;

                    grid[newcell_position.x, newcell_position.y, newcell_position.z] = true;
                    Cell newcell = new(newcell_position.x, newcell_position.y, newcell_position.z, false, cell.ro);
                    tempCells.Add(new Cell(cell));
                    tempCells.Add(newcell);
                    NonStemCellsAlive += 1;
                    CellsAlive += 1;
                }
            }
            else
                if (fate < pm)
            {
                grid[newcell_position.x, newcell_position.y, newcell_position.z] = grid[cell.x, cell.y, cell.z];
                grid[cell.x, cell.y, cell.z] = false;
                tempCells.Add(new Cell(newcell_position.x, newcell_position.y, newcell_position.z, false, cell.ro));

            }
            else
            {
                tempCells.Add(new Cell(cell));
            }


        }
        else
            tempCells.Add(new Cell(cell));


    }


    private int CountAllEmptyNeighbors()
    {
        int total_empty_neighbors = 0;

        foreach (Cell cell in cells)
        {
            total_empty_neighbors += GetEmptyNeighbors(cell.x, cell.y, cell.z).Count;
        }
        return total_empty_neighbors;
    }

    List<Vector3Int> GetEmptyNeighbors(int i, int j, int k)
    {
        List<Vector3Int> neighbors = new List<Vector3Int>();
        for (int x = -1; x <= 1; x++)
        {
            for (int y = -1; y <= 1; y++)
            {
                for (int z = -1; z <= 1; z++)
                {
                    if (x == 0 && y == 0 && z == 0)
                    {
                        continue;
                    }
                    if (i + x >= 0 && i + x < grid_size && j + y >= 0 && j + y < grid_size && k + z >= 0 && k + z < grid_size && grid[i + x, j + y, k + z] == false)
                    {
                        neighbors.Add(new Vector3Int(i + x, j + y, k + z));
                    }
                }
            }
        }
        return neighbors;
    }
    private void DestroyCells()
    {
        for (int cell = 0; cell < CellsAlive; cell++)
        {
            PreInstantiatedCells[cell].SetActive(false);
        }
    }

    private void Instantiate_cells()
    {
        MaterialPropertyBlock propertyBlock = new MaterialPropertyBlock();
        float rhoMax = nsc_traitvector["rho_max"];
        Vector3[] positions = new Vector3[cells.Count];
        Color[] colors = new Color[cells.Count];

        // Procesar datos en paralelo
        System.Threading.Tasks.Parallel.For(0, cells.Count, index =>
        {
            Cell cell = cells[index];
            positions[index] = new Vector3(cell.x, cell.y, cell.z);

            if (!cell.isStemCell)
            {
                float clampedValue = Mathf.Clamp(cell.ro, 0f, rhoMax);
                float normalizedValue = clampedValue / rhoMax;
                colors[index] = normalizedValue < 0.5f
                    ? Color.Lerp(Color.black, Color.blue, normalizedValue * 2)
                    : Color.Lerp(Color.blue, Color.red, (normalizedValue - 0.5f) * 2);
            }
            else
            {
                colors[index] = Color.clear; // Color especial para stem cells si no lo necesitas
            }
        });

        // Aplicar los datos procesados en el Main Thread
        for (int i = 0; i < cells.Count; i++)
        {
            GameObject nscell = PreInstantiatedCells[i];
            nscell.transform.position = positions[i];
            nscell.SetActive(true);

            if (!cells[i].isStemCell)
            {
                Renderer renderer = nscell.GetComponent<Renderer>();
                if (renderer != null)
                {
                    renderer.GetPropertyBlock(propertyBlock);
                    propertyBlock.SetColor("_BaseColor", colors[i]);
                    renderer.SetPropertyBlock(propertyBlock);
                }
            }
        }
    }


    //Version secuencial

    // private void Instantiate_cells()
    // {
    //     MaterialPropertyBlock propertyBlock = new MaterialPropertyBlock();
    //     int actual_cell = 0;

    //     foreach (Cell cell in cells)
    //     {
    //         int i = cell.x, j = cell.y, k = cell.z;
    //         GameObject nscell = PreInstantiatedCells[actual_cell];
    //         nscell.transform.position = new Vector3(i, j, k);
    //         nscell.SetActive(true);

    //         if (cell.isStemCell)
    //         {
    //             actual_cell++;
    //             continue;
    //         }

    //         // Calcular el color de la célula
    //         float clampedValue = Mathf.Clamp(cell.ro, 0f, nsc_traitvector["rho_max"]);
    //         float normalizedValue = clampedValue / nsc_traitvector["rho_max"];
    //         Color color = normalizedValue < 0.5f
    //             ? Color.Lerp(Color.blue, Color.white, normalizedValue * 2)
    //             : Color.Lerp(Color.white, Color.red, (normalizedValue - 0.5f) * 2);

    //         // Aplicar el color
    //         Renderer renderer = nscell.GetComponent<Renderer>();
    //         if (renderer != null)
    //         {
    //             renderer.GetPropertyBlock(propertyBlock);
    //             propertyBlock.SetColor("_BaseColor", color);
    //             renderer.SetPropertyBlock(propertyBlock);
    //         }
    //         else
    //         {
    //             UnityEngine.Debug.LogWarning($"El prefab '{nscell.name}' no tiene un Renderer.");
    //         }

    //         actual_cell++;
    //     }
    // }



}