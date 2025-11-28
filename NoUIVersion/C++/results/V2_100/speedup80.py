import os
import re
import numpy as np
import matplotlib.pyplot as plt

def parse_filename(filename):
    """Extrae el número de hilos del nombre del archivo."""
    match = re.search(r'simdata(\d+)threads', filename)
    if match:
        return int(match.group(1))
    return None

def extract_times_from_file(filepath):
    """Extrae los tiempos de ejecución del archivo (los valores después de :)"""
    times = []
    with open(filepath, 'r') as file:
        for line in file:
            if ':' in line:
                try:
                    _, time_str = line.strip().split(':')
                    times.append(float(time_str))
                except ValueError:
                    continue
    return times

def main():
    # Diccionario con hilos como clave y lista de tiempos como valor
    data = {}

    # Leer todos los archivos que empiecen con simdata
    for filename in os.listdir('.'):
        if filename.startswith('simdata') and 'threads' in filename:
            num_threads = parse_filename(filename)
            if num_threads is not None:
                times = extract_times_from_file(filename)
                if times:
                    data[num_threads] = times

    if 1 not in data:
        print("No se encontró archivo con 1 hilo para referencia de speedup.")
        return

    # Calcular tiempos medios
    mean_times = {threads: np.mean(times) for threads, times in data.items()}
    std_devs = {threads: np.std(times) for threads, times in data.items()}

    # Tiempo medio de ejecución con 1 hilo (para normalizar)
    baseline_time = mean_times[1]

    # Calcular speedups
    speedups = {threads: baseline_time / mean for threads, mean in mean_times.items()}
    # Ordenar por número de hilos
    sorted_threads = sorted(speedups.keys())
    speedup_values = [speedups[t] for t in sorted_threads]

    # Grafica de speedup medio
    plt.figure(figsize=(10, 6))
    plt.xticks([1] + list(range(5, 91, 5)))
    plt.errorbar(sorted_threads, speedup_values, fmt='o-', capsize=5, label="Speedup medio")
    #plt.errorbar(sorted_threads, speedup_values, yerr=speedup_errors, fmt='o-', capsize=5, label="Speedup medio")
    plt.xlabel("Número de hilos")
    plt.ylabel("Speedup medio")
    plt.title("Speedup medio vs Número de hilos")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig("speedup_vs_threads.png")
    plt.show()
    #grafica de tiempo medio junto a la desviacion tipica
    plt.figure(figsize=(10, 6))
    plt.xticks([1] + list(range(5, 91, 5)))
    plt.errorbar(sorted_threads, [mean_times[t] for t in sorted_threads], fmt='o-', capsize=5, label="Tiempo medio")
    plt.xlabel("Número de hilos")
    plt.ylabel("Tiempo medio (s)")
    plt.title("Tiempo medio vs Número de hilos")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig("tiempo_vs_threads.png")
    # Guarda los valores medios y errores en un archivo
    with open("speedup_results.txt", "w") as f:
        f.write("Número de hilos,Tiempo medio (s),Desviación estándar (s),Speedup\n")
        for threads in sorted_threads:
            f.write(f"{threads},{mean_times[threads]},{std_devs[threads]},{speedups[threads]}\n")
if __name__ == '__main__':
    main()
