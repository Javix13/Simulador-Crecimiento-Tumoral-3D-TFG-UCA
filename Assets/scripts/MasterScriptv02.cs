using System;
using System.Collections;
using System.Collections.Generic;
using System.Diagnostics;
using UnityEngine;
using UnityEngine.InputSystem;
using UnityEngine.UI;

public class MasterScriptv02 : MonoBehaviour
{
    public event System.Action<int, int, int, int> OnDayChanged;
    public event System.Action<bool> OnPause;

    public Dictionary<string, float> sc_traitvector = new Dictionary<string, float> {
        { "cct", 24 }, { "rho_max", 9 }, { "mu", 100 }, { "alpha", 0 }
    };

    public Dictionary<string, float> nsc_traitvector = new Dictionary<string, float> {
        { "cct", 24 }, { "rho_max", 9 }, { "mu", 100 }, { "alpha", 0.01f }
    };

    private float inct;
    private float pd;
    private float pm;
    private readonly float updateInterval = 0.2f;

    public float ps = 0.1f;
    public int grid_size;

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

    [Header("Indirect voxel rendering")]
    public Material voxelMaterial;
    public float voxelScale = 1.5f;

    private Mesh voxelMesh;
    private GraphicsBuffer voxelBuffer;
    private GraphicsBuffer argsBuffer;

    private GraphicsBuffer.IndirectDrawIndexedArgs[] indirectArgs = new GraphicsBuffer.IndirectDrawIndexedArgs[1];
    private Bounds drawBounds;

    private struct VoxelRenderData
    {
        public Vector4 posScale; // xyz = posición, w = escala
        public Vector4 color;    // rgba
    }

    private readonly List<VoxelRenderData> voxelData = new List<VoxelRenderData>();
    private readonly uint[] args = new uint[5];

    private void Awake()
    {
        nSteps = days * 24;
        grid = new bool[grid_size, grid_size, grid_size];
        inct = 1.30f;
        pd = 1.0f / 24.0f * inct;
        pm = 10.0f / 24.0f;

        cells = new List<Cell>();
        tempCells = new List<Cell>();

        int center = grid_size / 2;
        grid[center, center, center] = true;
        cells.Add(new Cell(center, center, center, true, (int)sc_traitvector["rho_max"]));

        CellsAlive = 1;
        StemCellsAlive = 1;
        NonStemCellsAlive = 0;

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

        SetupVoxelRenderer();
        UploadVoxelData();
    }

    private void SetupVoxelRenderer()
    {
        GameObject cube = GameObject.CreatePrimitive(PrimitiveType.Cube);
        voxelMesh = cube.GetComponent<MeshFilter>().sharedMesh;
        Destroy(cube);

        if (voxelMaterial != null)
            voxelMaterial.enableInstancing = true;

        Vector3 center = new Vector3((grid_size - 1) * 0.5f, (grid_size - 1) * 0.5f, (grid_size - 1) * 0.5f) * voxelScale;
        Vector3 size = new Vector3(grid_size, grid_size, grid_size) * voxelScale * 2f;
        drawBounds = new Bounds(center, size);
    }

    private void Start()
    {
        time = Time.realtimeSinceStartup;
        isSimulating = true;
        StartCoroutine(Simulate());
    }

    private void LateUpdate()
    {
        if (Input.GetKey("escape") && !isSimulating)
        {
            UnityEngine.SceneManagement.SceneManager.LoadScene(0);
        }
        if (Keyboard.current.spaceKey.wasPressedThisFrame)
        {
            isSimulating = !isSimulating;
            OnPause?.Invoke(isSimulating);
            if (isSimulating)
            {
                StartCoroutine(Simulate());
            }
        }

        DrawVoxels();
    }

    private IEnumerator Simulate()
    {

        while (isSimulating && step <= nSteps)
        {
            int totalCells = cells.Count;
            for (int i = 0; i < totalCells; i++)
            {
                Cell cell = cells[i];
                if (cell.isStemCell)
                {
                    Update_scells(cell);
                }

                else
                {
                    Update_nscells(cell);
                }
            }

            (tempCells, cells) = (cells, tempCells);
            tempCells.Clear();

            UploadVoxelData();

            yield return new WaitForSeconds(updateInterval);

            //if (step % 24 == 0)
            //{
            int day = (step / 24) + 1;
            OnDayChanged?.Invoke(day, CellsAlive, StemCellsAlive, NonStemCellsAlive);
            //}

            step++;
        }
    }

    private void UploadVoxelData()
    {
        voxelData.Clear();

        float rhoMax = nsc_traitvector["rho_max"];

        for (int i = 0; i < cells.Count; i++)
        {
            Cell cell = cells[i];
            Color color;

            if (cell.isStemCell)
            {
                color = Color.red;
            }
            else
            {
                float clampedValue = Mathf.Clamp(cell.ro, 0f, rhoMax);
                float normalizedValue = clampedValue / rhoMax;

                color = normalizedValue < 0.5f
                    ? Color.Lerp(new Color32(3, 39, 102, 255),      // Azul
            new Color32(235, 240, 248, 255),    // Blanco azulado
            normalizedValue * 2f)
                    : Color.Lerp(
                        new Color32(235, 240, 248, 255),    // Blanco azulado
            new Color32(164, 27, 38, 255),      // Rojo
            (normalizedValue - 0.5f) * 2f
                      );
            }

            voxelData.Add(new VoxelRenderData
            {
                posScale = new Vector4(
                    cell.x,
                    cell.y,
                    cell.z,
                    voxelScale
                ),
                color = color
            });
        }

        if (voxelData.Count == 0 ||
            voxelMaterial == null ||
            voxelMesh == null)
        {
            return;
        }

        EnsureBuffers(voxelData.Count);

        // Subir los datos de las células a GPU
        voxelBuffer.SetData(voxelData);

        // Configurar los argumentos del renderizado indirecto
        indirectArgs[0].indexCountPerInstance =
            voxelMesh.GetIndexCount(0);

        indirectArgs[0].instanceCount =
            (uint)voxelData.Count;

        indirectArgs[0].startIndex =
            voxelMesh.GetIndexStart(0);

        indirectArgs[0].baseVertexIndex =
            voxelMesh.GetBaseVertex(0);

        indirectArgs[0].startInstance = 0;

        argsBuffer.SetData(indirectArgs);

        voxelMaterial.SetBuffer("_VoxelBuffer", voxelBuffer);
    }

    private void EnsureBuffers(int count)
    {
        const int stride = sizeof(float) * 8; // posScale(4) + color(4)

        if (voxelBuffer == null || voxelBuffer.count < count)
        {
            ReleaseVoxelBuffer();

            voxelBuffer = new GraphicsBuffer(
                GraphicsBuffer.Target.Structured,
                count,
                stride
            );
        }

        if (argsBuffer == null)
        {
            argsBuffer = new GraphicsBuffer(
                GraphicsBuffer.Target.IndirectArguments,
                1,
                GraphicsBuffer.IndirectDrawIndexedArgs.size
            );
        }
    }

    private void DrawVoxels()
    {
        if (voxelMaterial == null ||
            voxelMesh == null ||
            argsBuffer == null ||
            voxelData.Count == 0)
        {
            return;
        }

        RenderParams rp = new RenderParams(voxelMaterial);
        rp.worldBounds = drawBounds;

        Graphics.RenderMeshIndirect(
            rp,
            voxelMesh,
            argsBuffer,
            1,
            0
        );
    }

    private void ReleaseVoxelBuffer()
    {
        if (voxelBuffer != null)
        {
            voxelBuffer.Release();
            voxelBuffer = null;
        }
    }

    private void ReleaseArgsBuffer()
    {
        if (argsBuffer != null)
        {
            argsBuffer.Release();
            argsBuffer = null;
        }
    }

    private void OnDestroy()
    {
        ReleaseVoxelBuffer();
        ReleaseArgsBuffer();
    }

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
}