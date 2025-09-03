using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class MenuController : MonoBehaviour
{
    [SerializeField] private GameObject mainMenu;
    [SerializeField] private GameObject settingsMenu;
    [SerializeField] private GameObject selectFileMenu;
    [SerializeField] private GameObject simulationParametersGO;
    [SerializeField] private TMPro.TMP_Dropdown simulationFilesDropdown;
    [SerializeField] private TMPro.TMP_InputField DaysInputFieldText;
    [SerializeField] private TMPro.TMP_InputField PSInputFieldText;
    [SerializeField] private TMPro.TMP_InputField RhoInputFieldText;
    [SerializeField] private TMPro.TMP_InputField AlphaInputFieldText;
    [SerializeField] private TMPro.TMP_InputField FilenameInputFieldText;
    [SerializeField] private TMPro.TMP_Text saveConfirmationText;
    private SimulationParameters simulationParameters;

    // Start is called once before the first execution of Update after the MonoBehaviour is created
    void Start()
    {
        mainMenu.SetActive(true);
        settingsMenu.SetActive(false);
        selectFileMenu.SetActive(false);
        simulationParameters = new SimulationParameters();



    }

    // Update is called once per frame
    void Update()
    {

    }

    public void OpenSelectFileMenu()
    {
        mainMenu.SetActive(false);
        selectFileMenu.SetActive(true);
        // load file names on dropdown
        simulationFilesDropdown.ClearOptions();
        string[] filePaths = System.IO.Directory.GetFiles(Application.persistentDataPath, "*.json");
        List<string> fileNames = new List<string>();
        foreach (string filePath in filePaths)
        {
            fileNames.Add(System.IO.Path.GetFileNameWithoutExtension(filePath));
        }
        simulationFilesDropdown.AddOptions(fileNames);
    }

    public void OpenSettingsMenu()
    {
        mainMenu.SetActive(false);
        settingsMenu.SetActive(true);
        selectFileMenu.SetActive(false);
    }

    public void CloseSettingsMenu()
    {
        mainMenu.SetActive(true);
        settingsMenu.SetActive(false);
        selectFileMenu.SetActive(false);
    }

    public void QuitGame()
    {
        Application.Quit();
    }

    public void StartSimulation()
    {
        //load JSON name from the dropdown
        string jsonName = simulationFilesDropdown.options[simulationFilesDropdown.value].text;
        // Load the JSON file and deserialize it into the simulation parameters
        string jsonPath = System.IO.Path.Combine(Application.persistentDataPath, jsonName + ".json");
        if (System.IO.File.Exists(jsonPath))
        {
            string json = System.IO.File.ReadAllText(jsonPath);
            simulationParameters = JsonUtility.FromJson<SimulationParameters>(json);
            simulationParametersGO.GetComponent<SimulationParametersGO>().nDays = simulationParameters.nDays;
            simulationParametersGO.GetComponent<SimulationParametersGO>().ps = simulationParameters.ps;
            simulationParametersGO.GetComponent<SimulationParametersGO>().rho = simulationParameters.rho;
            simulationParametersGO.GetComponent<SimulationParametersGO>().alpha = simulationParameters.alpha;
            DontDestroyOnLoad(simulationParametersGO);
            UnityEngine.SceneManagement.SceneManager.LoadScene(1);
        }
        // Start the simulation loading scene index 1 and send the simulation data to scene 1
        
    }


    public void getSimulationParametersFromMenu()
    {
        // verificate that all fields are filled
        if (string.IsNullOrEmpty(DaysInputFieldText.text) ||
            string.IsNullOrEmpty(PSInputFieldText.text) ||
            string.IsNullOrEmpty(RhoInputFieldText.text) ||
            string.IsNullOrEmpty(AlphaInputFieldText.text) ||
            string.IsNullOrEmpty(FilenameInputFieldText.text))
        {
            Debug.LogError("All fields must be filled");
            return;
        }
        // verficate nDays is between 1 and 999
        simulationParameters.nDays = int.Parse(DaysInputFieldText.text);
        if (simulationParameters.nDays < 1 || simulationParameters.nDays > 999)
        {
            Debug.LogError("Days must be between 1 and 999");
            return;
        }
        // verificate ps is between 0 and 100
        
        simulationParameters.ps = float.Parse(PSInputFieldText.text) / 100f;
        if (simulationParameters.ps < 0 || simulationParameters.ps > 1)
        {
            Debug.LogError("PS must be between 0 and 100");
            return;
        }

        simulationParameters.rho = int.Parse(RhoInputFieldText.text);


        simulationParameters.alpha = float.Parse(AlphaInputFieldText.text) / 100f;
        if (simulationParameters.alpha < 0 || simulationParameters.alpha > 1)
        {
            Debug.LogError("Alpha must be between 0 and 100");
            return;
        }
        simulationParameters.filename = FilenameInputFieldText.text;
        // Save data in a JSON file
        string json = JsonUtility.ToJson(simulationParameters);
        System.IO.File.WriteAllText(Application.persistentDataPath + "/" + simulationParameters.filename + ".json", json);

        // mostrar durante 2 segundos el texto de guardado con exito
        saveConfirmationText.gameObject.SetActive(true);
        Invoke("HideSaveConfirmationText", 2f);
    }

    private void HideSaveConfirmationText()
    {
        saveConfirmationText.gameObject.SetActive(false);
    }
}
