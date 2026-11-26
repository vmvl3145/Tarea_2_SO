#include "control_atomico_rw.hpp"
#include "control_rw.hpp"
#include "monitor_rw.hpp"
#include "verificador.hpp"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

// Muestra la sintaxis esperada del programa por stderr (R1).
void imprimir_uso(const char* prog) {
    std::cerr << "Uso: " << prog
              << " <modo: cond|atomic> <politica: lectores|escritores|fifo> "
                 "<L> <E> <K> <dur_lec_us> <dur_esc_us> [notraza]\n";
}

// Convierte una cadena completa a long rechazando basura final o desbordamiento (R1).
bool parsear_long_estricto(const char* texto, long& salida) {
    if (texto == nullptr || *texto == '\0') {
        return false;
    }
    errno = 0;
    char* fin = nullptr;
    const long val = std::strtol(texto, &fin, 10);
    if (errno == ERANGE || fin == texto || *fin != '\0') {
        return false;
    }
    salida = val;
    return true;
}

// Verifica a * b sin desbordar long (asumiendo a >= 0 y b >= 0) (R1).
bool multiplicar_seguro_long(long a, long b, long& salida) {
    if (a == 0 || b == 0) {
        salida = 0;
        return true;
    }
    if (a > std::numeric_limits<long>::max() / b) {
        return false;
    }
    salida = a * b;
    return true;
}

} // namespace

// Modulo A (Parte 1): CLI y validacion (R1), creacion/join de hebras y acceso a
// Registro (R2), conexion con MonitorRW::obtener_traza() y verificar_traza() para
// habilitar el trabajo del Modulo D.
int main(int argc, char* argv[]) {
    if (argc < 8 || argc > 9) {
        imprimir_uso(argv[0]);
        return 1;
    }

    // R1: validar modo y politica.
    const std::string modo = argv[1];
    const std::string politica_str = argv[2];

    if (modo != "cond" && modo != "atomic") {
        std::cerr << "Error: modo invalido '" << modo << "' (use cond o atomic).\n";
        return 1;
    }

    Politica politica = Politica::Lectores;
    if (politica_str == "lectores") {
        politica = Politica::Lectores;
    } else if (politica_str == "escritores") {
        politica = Politica::Escritores;
    } else if (politica_str == "fifo") {
        politica = Politica::Fifo;
    } else {
        std::cerr << "Error: politica invalida '" << politica_str
                  << "' (use lectores, escritores o fifo).\n";
        return 1;
    }

    // R1: atomic solo soporta prioridad de escritores.
    if (modo == "atomic" && politica != Politica::Escritores) {
        std::cerr << "Error: el modo atomic solo soporta la politica escritores.\n";
        return 1;
    }

    bool notraza = false;
    if (argc == 9) {
        const std::string opt = argv[8];
        if (opt != "notraza") {
            std::cerr << "Error: octavo argumento invalido '" << opt
                      << "' (solo se permite notraza).\n";
            return 1;
        }
        notraza = true;
    }

    // R1: parseo y validacion de rangos numericos.
    long L = 0;
    long E = 0;
    long K = 0;
    long dur_lec_us = 0;
    long dur_esc_us = 0;
    if (!parsear_long_estricto(argv[3], L) ||
        !parsear_long_estricto(argv[4], E) ||
        !parsear_long_estricto(argv[5], K) ||
        !parsear_long_estricto(argv[6], dur_lec_us) ||
        !parsear_long_estricto(argv[7], dur_esc_us)) {
        std::cerr << "Error: L, E, K, dur_lec_us y dur_esc_us deben ser enteros validos.\n";
        return 1;
    }

    if (L < 1 || E < 1 || L + E > 1000 || K < 1 || dur_lec_us < 0 || dur_esc_us < 0) {
        std::cerr << "Error: rangos invalidos (exige L>=1, E>=1, L+E<=1000, K>=1, "
                     "dur_lec_us>=0, dur_esc_us>=0).\n";
        return 1;
    }

    // R1: prevenir desbordamiento de long en 2*E*K y (L+E)*K antes de ejecutar.
    long dos_E = 0;
    long doble_escrituras = 0;
    long operaciones_totales = 0;
    if (!multiplicar_seguro_long(2, E, dos_E) ||
        !multiplicar_seguro_long(dos_E, K, doble_escrituras) ||
        !multiplicar_seguro_long(L + E, K, operaciones_totales)) {
        std::cerr << "Error: desbordamiento de long al calcular 2*E*K o (L+E)*K.\n";
        return 1;
    }
    const long escrituras_esperadas = E * K;

    // Instanciar la estrategia seleccionada mediante la interfaz comun ControlRW.
    MonitorRW* monitor_ptr = nullptr;
    std::unique_ptr<ControlRW> control;
    if (modo == "cond") {
        auto mon = std::make_unique<MonitorRW>(politica, !notraza);
        monitor_ptr = mon.get();
        control = std::move(mon);
    } else {
        control = std::make_unique<ControlAtomicoRW>();
    }

    Registro registro;

    // R2: contadores atomicos externos para diagnosticar exclusion mutua y
    // paralelismo maximo de lectoras sin depender de las clases de control.
    std::atomic<int> diag_lectoras_activas{0};
    std::atomic<int> diag_escritoras_activas{0};
    std::atomic<int> diag_max_lectoras{0};
    std::atomic<long> diag_inconsistencias{0};
    std::atomic<long> diag_solapamientos_escritura{0};

    // TODO(Modulo A - Parte 2):
    // 1. Medir tiempo total de pared (std::chrono::steady_clock) y tiempo de CPU del
    //    proceso (clock_gettime con CLOCK_PROCESS_CPUTIME_ID) alrededor de la creacion
    //    y join() de hebras (E2).
    // 2. Medir tiempo de espera de entrar_lectura()/entrar_escritura() por solicitud
    //    para reportar promedio y maximo separados por tipo (E2).
    // 3. Recolectar y sumar ControlAtomicoRW::estadisticas_hebra() tras join() (R10).

    std::vector<std::thread> hebras;
    hebras.reserve(static_cast<std::size_t>(L + E));

    bool fallo_creacion = false;
    std::string mensaje_error;

    try {
        // Crear L hebras lectoras con id_hebra en [0 .. L-1] (R1 y R6).
        for (long i = 0; i < L; ++i) {
            const int mi_id = static_cast<int>(i);
            hebras.emplace_back([&, mi_id]() {
                id_hebra = mi_id;

                for (long iter = 0; iter < K; ++iter) {
                    control->entrar_lectura();

                    // R2: diagnostico externo de lectoras activas y exclusion con escritoras.
                    const int activas = diag_lectoras_activas.fetch_add(1) + 1;
                    int max_prev = diag_max_lectoras.load();
                    while (activas > max_prev &&
                           !diag_max_lectoras.compare_exchange_weak(max_prev, activas)) {
                    }
                    if (diag_escritoras_activas.load() > 0) {
                        diag_solapamientos_escritura.fetch_add(1);
                    }

                    // R2: lectura del Registro y pausa simulada fuera de ControlRW.
                    const long v = registro.version;
                    const long dv = registro.doble_version;
                    if (dv != 2 * v) {
                        diag_inconsistencias.fetch_add(1);
                    }
                    if (dur_lec_us > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(dur_lec_us));
                    }

                    diag_lectoras_activas.fetch_sub(1);
                    control->salir_lectura();
                }
            });
        }

        // Crear E hebras escritoras con id_hebra en [L .. L+E-1] (R1 y R6).
        for (long j = 0; j < E; ++j) {
            const int mi_id = static_cast<int>(L + j);
            hebras.emplace_back([&, mi_id]() {
                id_hebra = mi_id;

                for (long iter = 0; iter < K; ++iter) {
                    control->entrar_escritura();

                    // R2: verificar que no haya otra escritora ni lectoras activas.
                    const int prev_e = diag_escritoras_activas.fetch_add(1);
                    if (prev_e > 0 || diag_lectoras_activas.load() > 0) {
                        diag_solapamientos_escritura.fetch_add(1);
                    }

                    // R2: actualizacion de Registro y pausa simulada fuera de ControlRW.
                    registro.version++;
                    registro.doble_version = 2 * registro.version;
                    if (dur_esc_us > 0) {
                        std::this_thread::sleep_for(std::chrono::microseconds(dur_esc_us));
                    }

                    diag_escritoras_activas.fetch_sub(1);
                    control->salir_escritura();
                }
            });
        }
    } catch (const std::system_error& e) {
        fallo_creacion = true;
        mensaje_error = e.what();
    }

    // R1: siempre esperar con join() a todas las hebras que si alcanzaron a crearse.
    for (std::thread& t : hebras) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (fallo_creacion) {
        std::cerr << "Error al crear hebras (" << hebras.size() << " de " << (L + E)
                  << " creadas): " << mensaje_error << "\n";
        return 1;
    }

    // R2: verificacion de invariantes finales.
    const long inconsistencias_totales =
        diag_inconsistencias.load() + diag_solapamientos_escritura.load();
    const bool correcto = (registro.version == escrituras_esperadas) &&
                          (registro.doble_version == doble_escrituras) &&
                          (inconsistencias_totales == 0);

    // Salida estandar base (en Parte 2 se agregan tiempos E2 y fallos R10).
    std::cout << "modo=" << modo
              << " politica=" << politica_str
              << " operaciones=" << operaciones_totales
              << " version=" << registro.version
              << " doble_version=" << registro.doble_version
              << " inconsistencias=" << inconsistencias_totales
              << " max_lectoras=" << diag_max_lectoras.load()
              << " correcto=" << (correcto ? "SI" : "NO");

    // R6: en modo cond con traza activa, invocar al verificador del Modulo D.
    if (modo == "cond" && !notraza && monitor_ptr != nullptr) {
        const ResultadoVerificacion ver =
            verificar_traza(monitor_ptr->obtener_traza(), politica);
        std::cout << " politica_ok=" << (ver.politica_ok ? "SI" : "NO")
                  << " evidencia=" << ver.evidencia;
        if (!ver.politica_ok && !ver.detalle.empty()) {
            std::cout << " detalle=\"" << ver.detalle << "\"";
        }
    }

    std::cout << "\n";
    return correcto ? 0 : 2;
}
