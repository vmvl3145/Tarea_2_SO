#ifndef MONITOR_RW_HPP
#define MONITOR_RW_HPP

#include "control_rw.hpp"

#include <condition_variable>
#include <mutex>
#include <vector>

// R3-R5
enum class Politica { Lectores, Escritores, Fifo };

// R6
enum class Tipo { Lectora, Escritora };

// R6
enum class Accion { Pide, Entra, Sale };

// R6
struct AnotacionTraza {
  int hebra;
  Tipo tipo;
  Accion accion;
  long turno; // turno FIFO; -1 en Lectores y Escritores
};

// Modulo B: Monitor de lectores y escritores basado en std::mutex y
// std::condition_variable (R3-R7).
// Todo el estado de admision debe mantenerse privado y protegido por el mutex
// m.
class MonitorRW : public ControlRW {
public:
  explicit MonitorRW(Politica p, bool registrar_traza = true);
  void entrar_lectura() override;
  void salir_lectura() override;
  void entrar_escritura() override;
  void salir_escritura() override;
  std::vector<AnotacionTraza> obtener_traza();

private:
  Politica politica;
  bool registrar_traza;
  std::mutex m;
  std::vector<AnotacionTraza> traza;

  // TODO(Modulo B): agregar estado de admision (activas, esperando, turnos
  // FIFO) y las std::condition_variable necesarias para R3-R7.
  std::condition_variable cv_lectoras;
  std::condition_variable cv_escritoras;
};

#endif
