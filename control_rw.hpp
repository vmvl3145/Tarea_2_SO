#ifndef CONTROL_RW_HPP
#define CONTROL_RW_HPP

// Permite que rw.cpp opere de forma uniforme sin conocer los detalles internos
// de MonitorRW ni de ControlAtomicoRW.
class ControlRW {
public:
  virtual ~ControlRW() = default;

  virtual void entrar_lectura() = 0; // Solicita acceso de lectura al recurso
                                     // compartido (puede bloquear/esperar).
  virtual void
  salir_lectura() = 0; // Libera el acceso de lectura al recurso compartido.
  virtual void entrar_escritura() = 0; // Solicita acceso exclusivo de escritura
                                       // al recurso compartido.

  virtual void salir_escritura() = 0; // Libera el acceso exclusivo de escritura
                                      // al recurso compartido.
};

// Invariante permanente fuera de escrituras activas: doble_version == 2 *
// version.
struct Registro {
  long version = 0;
  long doble_version = 0;
};

// Identificador logico por thread para registrar la traza de admision (R6).
// Cada thread creada en rw.cpp debe inicializarlo con un entero unico [0 ..
// L+E-1].
inline thread_local int id_hebra = -1;

#endif