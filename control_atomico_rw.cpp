#include "control_atomico_rw.hpp"

#include <thread>

EstadisticasFallosAtomicos &ControlAtomicoRW::estadisticas_hebra() {
  thread_local EstadisticasFallosAtomicos stats;
  return stats;
}

void ControlAtomicoRW::entrar_lectura() {

  auto &stats = estadisticas_hebra();

  while (true) {
    const std::uint64_t actual = estado.load();
    const bool escritora_activa = extraer_escritora_activa(actual) != 0;
    const bool hay_escritoras_esperando =
        extraer_escritoras_esperando(actual) != 0;

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
  estado.fetch_sub(UNIDAD_LECTORA);
}

void ControlAtomicoRW::entrar_escritura() {
  auto &stats = estadisticas_hebra();
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
  estado.fetch_sub(UNIDAD_ESCRITORA_ACTIVA);
}
