#pragma once

// Contrato de la salida de actuación de la alarma. En el demo es un único GPIO
// (placeholder); en producción serán 2 relés de estado sólido + sirena/estrobo.
//
// AlarmController depende de ESTA interfaz, no del GPIO concreto -> la máquina de
// estados se testea en env:native inyectando un fake (criterio "Por defecto":
// inyección de dependencias para la parte que sí se testea de forma aislada).
class IAlarmOutput {
public:
  virtual ~IAlarmOutput() = default;
  virtual void activate()   = 0;  // energiza la salida de disparo
  virtual void deactivate() = 0;  // la desactiva
};
