#ifndef VERIFICADOR_HPP
#define VERIFICADOR_HPP

#include "monitor_rw.hpp"

#include <string>
#include <vector>

// Resultado de la auditoria sobre la traza de admision de MonitorRW (R6 y E1).
struct ResultadoVerificacion {
  bool politica_ok = false;
  std::string evidencia; // Indicador resumido para imprimir en stdout
  std::string detalle;   // Explicaciхn legible en caso de fallo o validacion
};

// Modulo D: Verifica que la traza registrada cumpla:
// 1) Secuencia valida Pide -> Entra -> Sale por hebra y cierre limpio.
// 2) Exclusion mutua de escritura y coherencia lectura/escritura.
// 3) Reglas especificas de la politica (Lectores, Escritores o Fifo).
ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza> &traza,
                                      Politica politica);

#endif // VERIFICADOR_HPP
