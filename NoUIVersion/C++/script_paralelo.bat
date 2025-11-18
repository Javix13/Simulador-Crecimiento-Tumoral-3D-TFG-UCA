@echo off
echo Iniciando simulaciones paralelas...
g++ TumorPOOV2Paralelo.cpp -o TumorParalelo -std=c++20
for /L %%i in (2,2,16) do (
    echo Ejecutando simulación con %%i hilos...
    
    for /L %%j in (1,1,5) do (
	
        echo - Simulación %%j con %%i hilos...
        TumorParalelo -th %%i
        echo - Simulación %%j con %%i hilos finalizada.
    )
    
    echo Simulaciones con %%i hilos terminadas.
)

echo Todas las simulaciones han terminado.
pause