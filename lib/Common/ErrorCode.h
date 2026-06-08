#pragma once

// Códigos de error del firmware. Cada initialize() de un driver devolverá uno de
// estos en lugar de un bool (criterio "Por defecto" de la skill de firmware).
enum class ErrorCode {
  OK,
  SD_ERROR,
  GSM_ERROR,
  AUDIO_ERROR,
  CONFIG_ERROR,
};
