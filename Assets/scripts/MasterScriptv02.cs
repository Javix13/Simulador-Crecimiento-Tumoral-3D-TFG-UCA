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
    public Dictionary<string, float> sc_traitvector = new Dictionary<string, float> {
        { "cct", 24 }, { "rho_max", 10 }, { "mu", 100 }, { "alpha", 0 }
    };
    public Dictionary<string, float> nsc_traitvector = new Dictionary<string, float> {
        { "cct", 24 }, { "rho_max", 10 }, { "mu", 100 }, { "alpha", 0.01f }
    };

    private float inct;
    private float pd;
    private float pm;
    private readonly float updateInterval = 0.05f;
    public float ps = 0.1f;
    public int grid_size;
    public GameObject scellPrefab;
    public GameObject nscellPrefab;
    public Material NonStemCellExMaterial;
    public GameObject camera;
    private List<GameObject> PreInstantiatedCells;
    public Vector3Int[] cameraPositions;
    public int currentCameraPosition = 0;
    public int CellsAlive;
    public int StemCellsAlive;
    public int NonStemCellsAlive;
    public int step = 0;
    public int nSteps = 1440;

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
        grid = new bool[grid_size, grid_size, grid_size];
        inct = 1.30f;
        pd = 1.0f / 24.0f*inct;
        pm = 10.0f / 24.0f;
        cells = new List<Cell>();
        tempCells = new List<Cell>();
        PreInstantiatedCells = new List<GameObject>();
        int center = grid_size / 2;
        grid[center, center, center] = true;
        cells.Add(new Cell(center, center, center, true, (int)sc_traitvector["rho_max"]));
        cameraPositions = new Vector3Int[4];
        cameraPositions[0] = new Vector3Int(center, center, center - grid_size * 2);
        cameraPositions[1] = new Vector3Int(center + grid_size * 2, center, center);
        cameraPositions[2] = new Vector3Int(center, center, center+grid_size * 2);
        cameraPositions[3] = new Vector3Int(center - grid_size * 2, center, center);
        camera.transform.position = cameraPositions[0];
        CellsAlive = 1;
        StemCellsAlive = 1;
        NonStemCellsAlive = 0;
        //UnityEngine.Random.InitState(0);

    }
    private void Start() {
        time = Time.realtimeSinceStartup;
        for (int i=1;i<300000;i++){
            GameObject cell = Instantiate(scellPrefab, new Vector3(0, 0, 0), Quaternion.identity);
            cell.SetActive(false);
            PreInstantiatedCells.Add(cell);
        }
        isSimulating = true;
        StartCoroutine(Simulate());
    }

    private void Update() {
        if (isSimulating)
        {
            text.text = "Step: " + (step+1) + "\n" + "Time: " + (Time.realtimeSinceStartup - time).ToString("F2") + "s" + "\n" + "Cells Alive: " + CellsAlive + "\n" + "Stem Cells Alive: " + StemCellsAlive + "\n" + "Non Stem Cells Alive: " + NonStemCellsAlive;
        }
        

        
       // si el usuario pulsa la d muevete hacia la derecha del vector de posiciones de la camara;
        if (Keyboard.current.dKey.wasPressedThisFrame)
        {
            currentCameraPosition = (currentCameraPosition + 1) % cameraPositions.Length;
            camera.transform.position = cameraPositions[currentCameraPosition];
            camera.transform.Rotate(0, -90, 0);
            
        }
        // si el usuario pulsa la a muevete hacia la izquierda del vector de posiciones de la camara;
        if (Keyboard.current.aKey.wasPressedThisFrame)
        {
            if (currentCameraPosition == 0)
            {
                currentCameraPosition = cameraPositions.Length-1;
            }
            currentCameraPosition = (currentCameraPosition - 1) % cameraPositions.Length;
            camera.transform.position = cameraPositions[currentCameraPosition];
            camera.transform.Rotate(0, 90, 0);
        }

    }

    private IEnumerator Simulate()
    {
        for (step = 0; step < nSteps; step++)
        {
            if(step % 24 == 0)
                DestroyCells();
            UpdateState();
            
            
            yield return new WaitForSeconds(updateInterval);
            
        }
        isSimulating = false;
       
    }
    private void UpdateState()
    {
        stopwatch2.Start();
        while (cells.Count > 0){
            Cell cell = cells.ElementAt(UnityEngine.Random.Range(0, cells.Count-1));
            
            if (cell.isStemCell)
            {
                Update_scells(cell);
            }
            else
            {
                Update_nscells(cell);
            }
            cells.Remove(cell);
        }
        stopwatch2.Stop();
        UnityEngine.Debug.Log("UpdateState: " + stopwatch2.Elapsed);
        stopwatch2.Reset();
        // foreach (Cell cell in cells)
        // {
        //     int i = cell.x, j = cell.y, k = cell.z;
        //     if (cell.isStemCell)
        //     {
        //         Update_scells(cell);

        //     }
        //     else
        //     {
        //         Update_nscells(cell);
        //     }
        // }
        //copy tempCells to cells
        cells = new List<Cell>(tempCells);
        tempCells.Clear();
        if(step % 24 == 0)
            Instantiate_cells();
        

    }

    private void Update_scells(Cell cell)
    {
        float pr_random = UnityEngine.Random.Range(0.0f, 1.0f);
        List<Vector3Int> neighbors = GetEmptyNeighbors(cell.x, cell.y, cell.z);
        if (neighbors.Count > 0)
        {

            if (pr_random < pd)
            {
                float ps_random = UnityEngine.Random.Range(0.0f, 1.0f);
                Vector3Int position = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                if (ps_random < ps)
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
                if (pr_random < pm)
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
        float r = UnityEngine.Random.Range(0.0f, 1.0f);
        if (r < nsc_traitvector["alpha"])
        {
            grid[cell.x, cell.y, cell.z] = false;
            NonStemCellsAlive -= 1;
            CellsAlive -= 1;
            return;
        }
        List<Vector3Int> neighbors = GetEmptyNeighbors(cell.x, cell.y, cell.z);
        if (neighbors.Count > 0)
        {
            float pr_random = UnityEngine.Random.Range(0.0f, 1.0f);
            if (pr_random < pd && cell.ro!=0)
            {
                cell.ro -= 1;
                Vector3Int newcell_position = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                grid[newcell_position.x, newcell_position.y, newcell_position.z] = true;
                Cell newcell = new(newcell_position.x, newcell_position.y, newcell_position.z, false, cell.ro);
                tempCells.Add(new Cell(cell));
                tempCells.Add(newcell);
                NonStemCellsAlive += 1;
                CellsAlive += 1;
            }
            else
            {
                if (cell.ro == 0)
                {
                    grid[cell.x, cell.y, cell.z] = false;
                    NonStemCellsAlive -= 1;
                    CellsAlive -= 1;
                    return;
                }
                if (pr_random < pm)
                {
                    Vector3Int newcoor = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                    grid[newcoor.x, newcoor.y, newcoor.z] = grid[cell.x, cell.y, cell.z];
                    grid[cell.x, cell.y, cell.z] = false;
                    tempCells.Add(new Cell(newcoor.x, newcoor.y, newcoor.z, false, cell.ro));

                }
                else
                {
                    tempCells.Add(new Cell(cell));
                }
            }

        }
        else
            tempCells.Add(new Cell(cell));


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
        for (int cell=0;cell<CellsAlive;cell++)
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
                ? Color.Lerp(Color.blue, Color.white, normalizedValue * 2)
                : Color.Lerp(Color.white, Color.red, (normalizedValue - 0.5f) * 2);
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