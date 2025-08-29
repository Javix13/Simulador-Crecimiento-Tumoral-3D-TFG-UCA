using UnityEngine;
using UnityEngine.UI;


public class UIControl : MonoBehaviour
{
    [SerializeField] private MasterScriptv02 simulation_master;
    [SerializeField] private Text text;

    void OnEnable()
    {
        if (simulation_master != null)
            simulation_master.OnDayChanged += UpdateUI;
    }

    void OnDisable()
    {
        if (simulation_master != null)
            simulation_master.OnDayChanged -= UpdateUI;
    }

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {

    }

    // Update is called once per frame
    void Update()
    {

    }

    public void UpdateUI(int day, int cells_alive, int stem_cells_alive, int non_stem_cells_alive)
    {
        text.text = "Day: " + day + "\n" + "Cells Alive: " + cells_alive + "\n" + "Stem Cells Alive: " + stem_cells_alive + "\n" + "Non Stem Cells Alive: " + non_stem_cells_alive;
    }
}
