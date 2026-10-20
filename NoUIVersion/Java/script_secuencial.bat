@echo off
javac SimulacionTumoral.java
echo Iniciando simulaciones secuenciales...

for /L %%i in (1,1,10) do (
    echo Ejecutando simulación %%i...
    java SimulacionTumoral
    echo Simulación %%i finalizada.
)

echo Todas las simulaciones han terminado.
pause
