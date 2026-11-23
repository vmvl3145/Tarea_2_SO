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

  // ---------------------------------------------
  // Formato de la palabra unica de sincronizacion (std::uint64_t):
    // - Bits [15:0]  (16 bits): lectoras_activas     (mascara 0xFFFFULL,     desplazamiento 0)
    // - Bit  [16]    (1 bit)  : escritora_activa     (mascara 0x1ULL,        desplazamiento 16)
    // - Bits [32:17] (16 bits): escritoras_esperando (mascara 0xFFFFULL,     desplazamiento 17)
    // - Bits [63:33] (31 bits): reservados / no usados (0)
    //
    // Justificacion sin desborde (R1 y R9):
    // Con L + E <= 1000, el maximo posible de lectoras simultaneas es <= 1000
    // y el maximo de escritoras esperando simultaneamente es <= 1000. Cada campo
    // de 16 bits admite hasta 2^16 - 1 = 65535 (>= 1000), y escritora_activa es
    // un bit binario (0 o 1). Ningun campo puede desbordarse.
    static constexpr unsigned int SHIFT_LECTORAS = 0;
    static constexpr unsigned int SHIFT_ESCRITORA_ACTIVA = 16;
    static constexpr unsigned int SHIFT_ESCRITORAS_ESPERANDO = 17;

    static constexpr std::uint64_t MASCARA_CAMPO_16 = 0xFFFFULL;
    static constexpr std::uint64_t MASCARA_LECTORAS =
        MASCARA_CAMPO_16 << SHIFT_LECTORAS;
    static constexpr std::uint64_t MASCARA_ESCRITORA_ACTIVA =
        0x1ULL << SHIFT_ESCRITORA_ACTIVA;
    static constexpr std::uint64_t MASCARA_ESCRITORAS_ESPERANDO =
        MASCARA_CAMPO_16 << SHIFT_ESCRITORAS_ESPERANDO;

    static constexpr std::uint64_t UNIDAD_LECTORA = 1ULL << SHIFT_LECTORAS;
    static constexpr std::uint64_t UNIDAD_ESCRITORA_ACTIVA =
        1ULL << SHIFT_ESCRITORA_ACTIVA;
    static constexpr std::uint64_t UNIDAD_ESCRITORA_ESPERANDO =
        1ULL << SHIFT_ESCRITORAS_ESPERANDO;

    static constexpr std::uint64_t extraer_lectoras(std::uint64_t s) {
      return (s & MASCARA_LECTORAS) >> SHIFT_LECTORAS;
    }

    static constexpr std::uint64_t extraer_escritora_activa(std::uint64_t s) {
      return (s & MASCARA_ESCRITORA_ACTIVA) >> SHIFT_ESCRITORA_ACTIVA;
    }

    static constexpr std::uint64_t extraer_escritoras_esperando(std::uint64_t s) {
      return (s & MASCARA_ESCRITORAS_ESPERANDO) >> SHIFT_ESCRITORAS_ESPERANDO;
    }

  // ---------------------------------------------
  std::atomic<std::uint64_t> estado{0};
};

#endif // CONTROL_ATOMICO_RW_HPP
