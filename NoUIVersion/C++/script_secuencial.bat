@echo off
g++ TumorPOO.cpp -o TumorPOO -std=c++20
echo Iniciando simulaciones secuenciales...

for /L %%i in (1,1,5) do (
    echo Ejecutando simulación %%i...
    TumorPOO
    echo Simulación %%i finalizada.
)

echo Todas las simulaciones han terminado.
pause
