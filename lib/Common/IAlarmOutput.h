#pragma once

// Contrato (puerto) de la salida de actuación de la alarma. En el demo es un
// único GPIO (placeholder); en producción serán 2 relés de estado sólido +
// sirena/estrobo.
//
// Convención del proyecto: las INTERFACES que consume AlarmController viven en
// lib/Common (contratos puros, sin Arduino). Así la máquina de estados se testea
// en env:native sin arrastrar los drivers concretos (DFPlayer, TinyGSM, SD).
class IAlarmOutput {
public:
  virtual ~IAlarmOutput() = default;
  virtual void activate()   = 0;  // energiza la salida de disparo
  virtual void deactivate() = 0;  // la desactiva
};
