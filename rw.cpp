#include "control_atomico_rw.hpp"
#include "control_rw.hpp"
#include "monitor_rw.hpp"
#include "verificador.hpp"

#include <iostream>
#include <string>

namespace {

void imprimir_uso(const char* prog) {
    std::cerr << "Uso: " << prog
              << " <modo: cond|atomic> <politica: lectores|escritores|fifo> "
                 "<L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]\n";
}

} // namespace

// Modulo A: CLI, validacion de parametros (R1), ejecucion concurrente sobre Registro (R2),
// instrumentacion experimental (E2) y resumen estandar en stdout.
int main(int argc, char* argv[]) {
    if (argc < 8 || argc > 9) {
        imprimir_uso(argv[0]);
        return 1;
    }

    const std::string modo = argv[1];
    const std::string politica_str = argv[2];
    (void)modo;
    (void)politica_str;

    // TODO(Modulo A):
    // 1. Validar modo (cond | atomic), politica (lectores | escritores | fifo),
    //    [notraza] opcional, y rechazar modo == "atomic" con politica != "escritores" (R1).
    // 2. Parsear y validar L >= 1, E >= 1, L + E <= 1000, K >= 1, dur_lec_us >= 0,
    //    dur_esc_us >= 0, y que 2 * E * K y (L + E) * K no desborden un long (R1).
    // 3. Instanciar MonitorRW o ControlAtomicoRW segun modo usando ControlRW.
    // 4. Medir tiempo de pared (std::chrono::steady_clock) y tiempo de CPU del proceso
    //    (clock_gettime con CLOCK_PROCESS_CPUTIME_ID) alrededor de trabajo/join (E2).
    // 5. Crear L + E hebras std::thread asignando id_hebra unico al iniciar cada una;
    //    si falla la creacion, informar error y hacer join() a las ya creadas antes de salir (R1).
    // 6. En cada hebra ejecutar K iteraciones accediendo a Registro y sleep_for (si dur > 0)
    //    fuera de ControlRW, registrando contadores atomicos externos de diagnostico (R2),
    //    tiempos de espera por solicitud (E2) y fallos atomicos por hebra (R10).
    // 7. Verificar invariantes finales (version == E*K, doble_version == 2*E*K,
    //    inconsistencias == 0), llamar a verificar_traza si aplica e imprimir resumen en stdout.

    std::cout << "Esqueleto base ./rw compilado. Pendiente implementacion de modulos.\n";
    return 0;
}
