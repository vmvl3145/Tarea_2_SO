#include "control_atomico_rw.hpp"

#include <thread>

EstadisticasFallosAtomicos &ControlAtomicoRW::estadisticas_hebra() {
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

  auto &stats = estadisticas_hebra();

  while (true) {
    const std::uint64_t actual = estado.load();
    const bool escritora_activa = extraer_escritora_activa(actual) != 0;
    const bool hay_escritoras_esperando =
        extraer_escritoras_esperando(actual) != 0;

    // Prioridad de escritores: si hay escritora activa o escritoras esperando,
    // una nueva lectora no puede ingresar. Si coexisten ambas causas, se cuenta
    // como recurso_ocupado (R10).
    if (escritora_activa || hay_escritoras_esperando) {
      if (escritora_activa) {
        stats.recurso_ocupado++;
      } else {
        stats.escritoras_esperando++;
      }
      std::this_thread::yield();
      continue;
    }

    std::uint64_t esperado = actual;
    const std::uint64_t nuevo = actual + UNIDAD_LECTORA;

    if (estado.compare_exchange_weak(esperado, nuevo)) {
      return;
    }

    stats.cas_fallido++;
    std::this_thread::yield();
  }
}

void ControlAtomicoRW::salir_lectura() {
    // TODO(Modulo C):
    // Decrementar lectoras_activas atomicamente con fetch_sub (R9).
  estado.fetch_sub(UNIDAD_LECTORA);
}

void ControlAtomicoRW::entrar_escritura() {
  auto &stats = estadisticas_hebra();
      // TODO(Modulo C):
      // 1. Incrementar escritoras_esperando con fetch_add antes de competir por la entrada,
      //    bloqueando nuevas lectoras desde ese instante (R9).
      // 2. En bucle con load(), esperar hasta lectoras_activas == 0 && escritora_activa == 0;
      //    si esta ocupado, incrementar recurso_ocupado (R10) y hacer std::this_thread::yield().
      // 3. Al entrar, decrementar escritoras_esperando y activar escritora_activa en el mismo
      //    compare_exchange_weak(esperado, nuevo); si falla el CAS, incrementar cas_fallido (R9-R10).

  // Registrar primero que hay una escritora esperando para bloquear nuevas
  // lectoras desde este instante (R9).
  estado.fetch_add(UNIDAD_ESCRITORA_ESPERANDO);

  while (true) {
    const std::uint64_t actual = estado.load();
    const bool hay_lectoras_activas = extraer_lectoras(actual) != 0;
    const bool hay_escritora_activa = extraer_escritora_activa(actual) != 0;

    if (hay_lectoras_activas || hay_escritora_activa) {
      stats.recurso_ocupado++;
      std::this_thread::yield();
      continue;
    }

    // Punto de decision atomico: decrementa escritoras_esperando y marca
    // escritora_activa en el mismo CAS (R9).
    std::uint64_t esperado = actual;
    const std::uint64_t nuevo =
        (actual - UNIDAD_ESCRITORA_ESPERANDO) | UNIDAD_ESCRITORA_ACTIVA;

    if (estado.compare_exchange_weak(esperado, nuevo)) {
      return;
    }

    stats.cas_fallido++;
    std::this_thread::yield();
  }
}

void ControlAtomicoRW::salir_escritura() {
        // TODO(Modulo C):
        // Limpiar el bit de escritora_activa atomicamente con fetch_sub o CAS (R9).
  estado.fetch_sub(UNIDAD_ESCRITORA_ACTIVA);
}
