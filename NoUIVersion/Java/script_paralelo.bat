@echo off
echo Iniciando simulaciones paralelas...
javac SimulacionTumoralParalela.java
 for /L %%j in (1,1,5) do (
        java SimulacionTumoralParalela 2
        echo - Simulación %%j con 2 hilos finalizada. )
for /L %%i in (4,4,12) do (
    echo Ejecutando simulación con %%i hilos...
    
    for /L %%j in (1,1,5) do (
	
        echo - Simulación %%j con %%i hilos...
        java SimulacionTumoralParalela %%i
        echo - Simulación %%j con %%i hilos finalizada.
    )
    
    echo Simulaciones con %%i hilos terminadas.
)

echo Todas las simulaciones han terminado.
pause