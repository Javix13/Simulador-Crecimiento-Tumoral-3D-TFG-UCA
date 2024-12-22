using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.InputSystem;
public class MasterScriptv01 : MonoBehaviour
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
    private float updateInterval = 0.05f;
    public float ps = 0.1f;
    public int grid_size;
    public GameObject scellPrefab;
    public GameObject nscellPrefab;
    public Material NonStemCellExMaterial;
    public GameObject camera;

    public Vector3Int[] cameraPositions;
    public int currentCameraPosition = 0;
    public int CellsAlive;
    public int StemCellsAlive;
    public int NonStemCellsAlive;
    public int step = 0;
    public int nSteps = 1440;



    int[,,] grid;
    List<Vector3Int> cells;
    List<Vector3Int> tempCells;
    private void Awake()
    {
        grid = new int[grid_size, grid_size, grid_size];
        inct = 1.0f / sc_traitvector["cct"];
        pd = 24.0f / sc_traitvector["cct"] * inct * 1.30f;
        pm = 10.0f / 24.0f;
        cells = new List<Vector3Int>();
        tempCells = new List<Vector3Int>();
        int center = grid_size / 2;
        grid[center, center, center] = -1;
        cells.Add(new Vector3Int(center, center, center));
        cameraPositions = new Vector3Int[4];
        cameraPositions[0] = new Vector3Int(center, center, center - grid_size * 2);
        cameraPositions[1] = new Vector3Int(center + grid_size * 2, center, center);
        cameraPositions[2] = new Vector3Int(center, center, center+grid_size * 2);
        cameraPositions[3] = new Vector3Int(center - grid_size * 2, center, center);
        camera.transform.position = cameraPositions[0];
        CellsAlive = 1;
        StemCellsAlive = 1;
        NonStemCellsAlive = 0;

    }
    private void OnEnable()
    {
        StartCoroutine(Simulate());
    }

    private void Update() {
        
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
            DestroyCells();
            UpdateState();
            yield return new WaitForSeconds(updateInterval);
        }
    }
    private void UpdateState()
    {
        foreach (Vector3Int cell in cells)
        {
            int i = cell.x, j = cell.y, k = cell.z;
            if (grid[i, j, k] == -1)
            {
                Update_scells(i, j, k);

            }
            else
            {
                Update_nscells(i, j, k);
            }
        }
        cells.Clear();
        cells.AddRange(tempCells);
        tempCells.Clear();
        Instantiate_cells();


    }

    private void Update_scells(int i, int j, int k)
    {
        float pr_random = UnityEngine.Random.Range(0.0f, 1.0f);
        List<Vector3Int> neighbors = GetEmptyNeighbors(i, j, k);
        if (neighbors.Count > 0)
        {

            if (pr_random < pd)
            {
                float ps_random = UnityEngine.Random.Range(0.0f, 1.0f);
                Vector3Int position = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                if (ps_random < ps)
                {
                    grid[position.x, position.y, position.z] = -1;
                    tempCells.Add(position);
                    tempCells.Add(new Vector3Int(i, j, k));
                    StemCellsAlive += 1;
                }
                else
                {
                    grid[position.x, position.y, position.z] = (int)nsc_traitvector["rho_max"];
                    tempCells.Add(position);
                    tempCells.Add(new Vector3Int(i, j, k));
                    NonStemCellsAlive += 1;
                }

            }
            else
            {
                if (pr_random < pm)
                {
                    Vector3Int newcoor = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                    grid[newcoor.x, newcoor.y, newcoor.z] = grid[i, j, k];
                    tempCells.Add(newcoor);

                }
                else
                {
                    tempCells.Add(new Vector3Int(i, j, k));
                }
            }
        }
        else
        {
            tempCells.Add(new Vector3Int(i, j, k));
        }


    }

    private void Update_nscells(int i, int j, int k)
    {
        float r = UnityEngine.Random.Range(0.0f, 1.0f);
        if (r < nsc_traitvector["alpha"])
        {
            grid[i, j, k] = 0;
            NonStemCellsAlive -= 1;
            return;
        }
        List<Vector3Int> neighbors = GetEmptyNeighbors(i, j, k);
        if (neighbors.Count > 0)
        {
            float pr_random = UnityEngine.Random.Range(0.0f, 1.0f);
            if (pr_random < pd && grid[i, j, k] > 1)
            {
                grid[i, j, k] -= 1;
                Vector3Int newcell = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                grid[newcell.x, newcell.y, newcell.z] = grid[i, j, k];
                tempCells.Add(newcell);
                tempCells.Add(new Vector3Int(i, j, k));
                NonStemCellsAlive += 1;
            }
            else
            {
                if (grid[i, j, k] == 1)
                {
                    grid[i, j, k] = 0;
                    NonStemCellsAlive -= 1;
                    return;
                }
                if (pr_random < pm)
                {
                    Vector3Int newcoor = neighbors[UnityEngine.Random.Range(0, neighbors.Count)];
                    grid[newcoor.x, newcoor.y, newcoor.z] = grid[i, j, k];
                    grid[i, j, k] = 0;
                    tempCells.Add(newcoor);

                }
                else
                {
                    tempCells.Add(new Vector3Int(i, j, k));
                }
            }

        }
        else
            tempCells.Add(new Vector3Int(i, j, k));


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
                    if (i + x >= 0 && i + x < grid_size && j + y >= 0 && j + y < grid_size && k + z >= 0 && k + z < grid_size && grid[i + x, j + y, k + z] == 0)
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
        GameObject[] cells = GameObject.FindGameObjectsWithTag("cell");
        foreach (GameObject cell in cells)
        {
            Destroy(cell);
        }
    }

    private void Instantiate_cells()
{
    MaterialPropertyBlock propertyBlock = new MaterialPropertyBlock();

    foreach (Vector3Int cell in cells)
    {
        int i = cell.x, j = cell.y, k = cell.z;

        if (grid[i, j, k] == -1)
        {
            GameObject scell = Instantiate(scellPrefab, new Vector3(i, j, k), Quaternion.identity);
            scell.transform.SetParent(transform);
        }
        else
        {
            // Ajustar el valor de proliferación al rango permitido
            float clampedValue = Mathf.Clamp(grid[i, j, k], 0f, nsc_traitvector["rho_max"]);
            float normalizedValue = clampedValue / nsc_traitvector["rho_max"];

            // Calcular el color interpolado
            Color color;
            if (normalizedValue < 0.5f)
            {
                color = Color.Lerp(Color.blue, Color.white, normalizedValue * 2);
            }
            else
            {
                color = Color.Lerp(Color.white, Color.red, (normalizedValue - 0.5f) * 2);
            }

            // Instanciar la célula
            GameObject nscell = Instantiate(nscellPrefab, new Vector3(i, j, k), Quaternion.identity);
            nscell.transform.SetParent(transform);

            // Aplicar el color usando MaterialPropertyBlock
            Renderer renderer = nscell.GetComponent<Renderer>();
            if (renderer != null)
            {
                renderer.GetPropertyBlock(propertyBlock);
                propertyBlock.SetColor("_BaseColor", color);
                renderer.SetPropertyBlock(propertyBlock);
            }
            else
            {
                Debug.LogWarning($"El prefab '{nscellPrefab.name}' no tiene un Renderer.");
            }
        }
    }
}





}
