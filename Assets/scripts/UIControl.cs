using UnityEngine;
using UnityEngine.UI;


public class UIControl : MonoBehaviour
{
    [SerializeField] private MasterScriptv02 simulation_master;
    [SerializeField] private Text text;
    [SerializeField] public TMPro.TMP_Text pauseText;

    [SerializeField] public TMPro.TMP_Text exitText;

    void OnEnable()
    {
        if (simulation_master != null)
            simulation_master.OnDayChanged += UpdateUI;
            simulation_master.OnPause += UpdatePauseText;
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
        text.text = "Día: " + day + "\n" + "Células vivas: " + cells_alive + "\n" + "Células madre vivas: " + stem_cells_alive + "\n" + "Células no madre vivas: " + non_stem_cells_alive;
    }

    public void UpdatePauseText(bool isSimulating)
    {
        pauseText.gameObject.SetActive(!isSimulating);
        exitText.gameObject.SetActive(!isSimulating);
        
    }
}
