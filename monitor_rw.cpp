#include "monitor_rw.hpp"

MonitorRW::MonitorRW(Politica p, bool registrar_traza)
    : politica(p), registrar_traza(registrar_traza) {}

void MonitorRW::entrar_lectura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO(Modulo B):
    // 1. Si politica == Politica::Fifo, asignar turno consecutivo desde 0 (R5);
    //    en Lectores y Escritores usar turno = -1.
    // 2. Si registrar_traza es true, registrar Accion::Pide con id_hebra al adquirir m (R6).
    // 3. Esperar con cv_lectoras.wait(lock, predicado) segun politica (R3, R4, R5, R7):
    //    - Lectores: entra si no hay escritora activa (aunque haya escritoras esperando).
    //    - Escritores: entra si no hay escritora activa ni escritoras esperando.
    //    - Fifo: entra si no hay escritora activa ni escritora pendiente con turno anterior.
    // 4. Actualizar contadores de admision y registrar Accion::Entra antes de retornar (R6).
    (void)politica;
    if (registrar_traza) {
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Pide, -1});
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Entra, -1});
    }
}

void MonitorRW::salir_lectura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO(Modulo B):
    // 1. Decrementar contadores de lectoras activas.
    // 2. Si registrar_traza es true, registrar Accion::Sale (R6).
    // 3. Notificar mediante cv_escritoras / cv_lectoras sin dejar hebras habilitadas
    //    bloqueadas indefinidamente (R7).
    if (registrar_traza) {
        traza.push_back({id_hebra, Tipo::Lectora, Accion::Sale, -1});
    }
    cv_escritoras.notify_all();
    cv_lectoras.notify_all();
}

void MonitorRW::entrar_escritura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO(Modulo B):
    // 1. Si politica == Politica::Fifo, asignar turno consecutivo desde 0 (R5);
    //    en Lectores y Escritores usar turno = -1.
    // 2. Si registrar_traza es true, registrar Accion::Pide con id_hebra al adquirir m (R6).
    // 3. Esperar con cv_escritoras.wait(lock, predicado) segun politica (R3, R4, R5, R7):
    //    - Lectores: entra solo sin lectoras activas ni pendientes (y sin escritora activa).
    //    - Escritores: entra cuando no hay lectoras ni escritora activas.
    //    - Fifo: entra cuando no hay lectoras ni escritora activas y su turno es el menor pendiente.
    // 4. Actualizar contadores de admision y registrar Accion::Entra antes de retornar (R6).
    if (registrar_traza) {
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Pide, -1});
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Entra, -1});
    }
}

void MonitorRW::salir_escritura() {
    std::unique_lock<std::mutex> lock(m);

    // TODO(Modulo B):
    // 1. Liberar estado de escritora activa.
    // 2. Si registrar_traza es true, registrar Accion::Sale (R6).
    // 3. Notificar a escritoras y lectoras habilitadas segun la politica (R7).
    if (registrar_traza) {
        traza.push_back({id_hebra, Tipo::Escritora, Accion::Sale, -1});
    }
    cv_escritoras.notify_all();
    cv_lectoras.notify_all();
}

std::vector<AnotacionTraza> MonitorRW::obtener_traza() {
    std::unique_lock<std::mutex> lock(m);
    return traza;
}
