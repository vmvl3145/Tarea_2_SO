#include "verificador.hpp"

ResultadoVerificacion verificar_traza(const std::vector<AnotacionTraza>& traza,
                                      Politica politica) {
    ResultadoVerificacion res;

    // TODO(Modulo D):
    // 1. Validar que la traza no este vacia y que cada hebra siga el ciclo Pide -> Entra -> Sale,
    //    sin hebras pendientes ni activas al terminar (R6).
    // 2. Verificar exclusion mutua de escritoras y no solapamiento con lectoras.
    // 3. Validar reglas especificas segun politica:
    //    - Politica::Lectores   : escritora solo entra sin lectoras activas ni pendientes (R3).
    //    - Politica::Escritores : lectora no entra si hay escritoras activas o esperando (R4).
    //    - Politica::Fifo       : turnos consecutivos desde 0; lectoras consecutivas entran
    //                             simultaneamente salvo escritora pendiente con turno anterior (R5).
    // 4. Completar res.politica_ok, res.evidencia (indicador resumido) y res.detalle.
    (void)traza;
    (void)politica;

    res.politica_ok = false;
    res.evidencia = "pendiente";
    res.detalle = "Verificador aun no implementado (Modulo D)";
    return res;
}
