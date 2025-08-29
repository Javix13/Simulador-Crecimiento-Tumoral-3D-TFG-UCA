using UnityEngine;
using UnityEngine.InputSystem;

public class CameraControl : MonoBehaviour
{
    public Vector3Int[] cameraPositions;
    public int currentCameraPosition = 0;
    [SerializeField] private GameObject controlled_camera;
    [SerializeField] private GameObject simulation_master;
    private float zoom = 10f;
    private float camera_speed = 50f;
    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        int center = simulation_master.GetComponent<MasterScriptv02>().grid_size / 2;
        int grid_size = simulation_master.GetComponent<MasterScriptv02>().grid_size;
        cameraPositions = new Vector3Int[4];
        cameraPositions[0] = new Vector3Int(center, center, center - grid_size * 2);
        cameraPositions[1] = new Vector3Int(center + grid_size * 2, center, center);
        cameraPositions[2] = new Vector3Int(center, center, center + grid_size * 2);
        cameraPositions[3] = new Vector3Int(center - grid_size * 2, center, center);
        controlled_camera.transform.position = cameraPositions[0];
    }

    // Update is called once per frame
    void Update()
    {
        if (Keyboard.current.qKey.wasPressedThisFrame)
        {
            currentCameraPosition = (currentCameraPosition + 1) % cameraPositions.Length;
            controlled_camera.transform.position = cameraPositions[currentCameraPosition];
            controlled_camera.transform.Rotate(0, -90, 0);

        }
        // si el usuario pulsa la a muevete hacia la izquierda del vector de posiciones de la camara;
        if (Keyboard.current.eKey.wasPressedThisFrame)
        {
            if (currentCameraPosition == 0)
            {
                currentCameraPosition = cameraPositions.Length - 1;
            }
            else
                currentCameraPosition = (currentCameraPosition - 1) % cameraPositions.Length;

            controlled_camera.transform.position = cameraPositions[currentCameraPosition];
            controlled_camera.transform.Rotate(0, 90, 0);
        }
        //si el usuario gira el raton haz zoom
        if (Mouse.current.scroll.ReadValue().y > 0)
        {
            controlled_camera.transform.position += controlled_camera.transform.forward * zoom;
        }
        else if (Mouse.current.scroll.ReadValue().y < 0)
        {
            controlled_camera.transform.position -= controlled_camera.transform.forward * zoom;
        }
        // utiliza WASD para controlar la camara
        if (Keyboard.current.wKey.isPressed)
        {
            controlled_camera.transform.position += controlled_camera.transform.up * camera_speed * Time.deltaTime;
        }
        if (Keyboard.current.sKey.isPressed)
        {
            controlled_camera.transform.position -= controlled_camera.transform.up * camera_speed * Time.deltaTime;
        }
        if (Keyboard.current.aKey.isPressed)
        {
            controlled_camera.transform.position -= controlled_camera.transform.right * camera_speed * Time.deltaTime;
        }
        if (Keyboard.current.dKey.isPressed)
        {
            controlled_camera.transform.position += controlled_camera.transform.right * camera_speed * Time.deltaTime;
        }
    }
}
