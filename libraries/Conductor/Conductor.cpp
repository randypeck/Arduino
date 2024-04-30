// CONDUCTOR.CPP Rev: 02/27/23.
// Part of O_LEG.

#include "Conductor.h"

Conductor::Conductor() {  // Constructor
  return;
}

void Conductor::begin(FRAM * t_pStorage, Block_Reservation * t_pBlockReservation, Delayed_Action* t_pDelayedAction,
                      Engineer* t_pEngineer) {

  m_pStorage = t_pStorage;  // Pointer to FRAM
  m_pBlockReservation = t_pBlockReservation;  // Pointer to Block Reservation table
  m_pDelayedAction = t_pDelayedAction;  // So at Registration we can call Delayed_Action::initialize()
  m_pEngineer = t_pEngineer;            // So at Registration we can call Engineer::legacyCommandBufInit() and accessoryRelayInit()

  if ((m_pStorage == nullptr) || (m_pBlockReservation == nullptr) || (m_pDelayedAction == nullptr) || (m_pEngineer == nullptr)) {
    sprintf(lcdString, "UN-INIT'd CD PTR"); pLCD2004->println(lcdString); Serial.println(lcdString); endWithFlashingLED(5);
  }

  return;
}

void Conductor::conduct(const byte t_mode, byte* t_state) {  // Manage Train Progress and Delayed Action tables

  return;
}
