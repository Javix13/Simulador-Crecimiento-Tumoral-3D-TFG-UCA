@echo off
g++ SimulacionTumoral.cpp -o SimulacionTumoral -std=c++20
echo Iniciando simulaciones secuenciales...

for /L %%i in (1,1,10) do (
    echo Ejecutando simulación %%i...
    SimulacionTumoral
    echo Simulación %%i finalizada.
)

echo Todas las simulaciones han terminado.
pause
