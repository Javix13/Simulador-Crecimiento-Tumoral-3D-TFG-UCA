using UnityEngine;
public class Cell
{
    public int x;
    public int y;
    public int z;
    public bool isStemCell;
    public GameObject cellObject;
    public int ro;

    public Cell(int x, int y, int z, bool isStemCell, int ro)
    {
        this.x = x;
        this.y = y;
        this.z = z;
        this.isStemCell = isStemCell;
        this.ro = ro;
    }
    public Cell(Cell cell)
    {
        this.x = cell.x;
        this.y = cell.y;
        this.z = cell.z;
        this.isStemCell = cell.isStemCell;
        this.ro = cell.ro;
    }
    public void setGameObject(GameObject cellObject)
    {
        this.cellObject = cellObject;
    }
}
