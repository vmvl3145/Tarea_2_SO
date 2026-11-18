#include "control_atomico_rw.hpp"

#include <thread>

EstadisticasFallosAtomicos& ControlAtomicoRW::estadisticas_hebra() {
    thread_local EstadisticasFallosAtomicos stats;
    return stats;
}

void ControlAtomicoRW::entrar_lectura() {
    // TODO(Modulo C):
    // 1. Leer estado con estado.load().
    // 2. Verificar en la misma palabra que escritora_activa == 0 && escritoras_esperando == 0 (R8-R9).
    // 3. Si falla, incrementar estadisticas_hebra().recurso_ocupado (prioritario si coexisten causas)
    //    o estadisticas_hebra().escritoras_esperando (R10) y ceder CPU con std::this_thread::yield().
    // 4. Si puede entrar, incrementar lectoras_activas con compare_exchange_weak(esperado, nuevo)
    //    sin serializar lectoras con un candado; si el CAS falla, incrementar cas_fallido (R9-R10).
    (void)estado.load();
}

void ControlAtomicoRW::salir_lectura() {
    // TODO(Modulo C):
    // Decrementar lectoras_activas atomicamente con fetch_sub (R9).
}

void ControlAtomicoRW::entrar_escritura() {
    // TODO(Modulo C):
    // 1. Incrementar escritoras_esperando con fetch_add antes de competir por la entrada,
    //    bloqueando nuevas lectoras desde ese instante (R9).
    // 2. En bucle con load(), esperar hasta lectoras_activas == 0 && escritora_activa == 0;
    //    si esta ocupado, incrementar recurso_ocupado (R10) y hacer std::this_thread::yield().
    // 3. Al entrar, decrementar escritoras_esperando y activar escritora_activa en el mismo
    //    compare_exchange_weak(esperado, nuevo); si falla el CAS, incrementar cas_fallido (R9-R10).
    (void)estado.load();
}

void ControlAtomicoRW::salir_escritura() {
    // TODO(Modulo C):
    // Limpiar el bit de escritora_activa atomicamente con fetch_sub o CAS (R9).
}
