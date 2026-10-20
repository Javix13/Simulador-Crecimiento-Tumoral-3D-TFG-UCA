@echo off
echo Iniciando simulaciones paralelas...
g++ SimulacionTumoralParalela.cpp -o SimulacionParalela -std=c++20
for /L %%j in (1,1,5) do (
	
        echo - Simulación %%j con %%i hilos...
        SimulacionParalela -th 2
        echo - Simulación %%j con %%i hilos finalizada.
    )
for /L %%i in (4,4,12) do (
    echo Ejecutando simulación con %%i hilos...
    
    for /L %%j in (1,1,5) do (
	
        echo - Simulación %%j con %%i hilos...
        SimulacionParalela -th %%i
        echo - Simulación %%j con %%i hilos finalizada.
    )
    
    echo Simulaciones con %%i hilos terminadas.
)



echo Todas las simulaciones han terminado.
pause