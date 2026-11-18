#ifndef CONTROL_ATOMICO_RW_HPP
#define CONTROL_ATOMICO_RW_HPP

#include "control_rw.hpp"

#include <atomic>
#include <cstdint>

struct EstadisticasFallosAtomicos {
  std::uint64_t recurso_ocupado = 0;
  std::uint64_t escritoras_esperando = 0;
  std::uint64_t cas_fallido = 0;
};

class ControlAtomicoRW : public ControlRW {
public:
  ControlAtomicoRW() = default;
  void entrar_lectura() override;
  void salir_lectura() override;
  void entrar_escritura() override;
  void salir_escritura() override;
  static EstadisticasFallosAtomicos &estadisticas_hebra();

private:
  static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                "std::atomic<std::uint64_t> debe ser siempre lock-free");

  // TODO(Modulo C): definir y documentar las mascaras de bits sobre esta
  // palabra unica para empaquetar lectoras_activas, escritora_activa y
  // escritoras_esperando, justificando que no hay desborde con L + E <= 1000
  // (R9).
  std::atomic<std::uint64_t> estado{0};
};

#endif // CONTROL_ATOMICO_RW_HPP
