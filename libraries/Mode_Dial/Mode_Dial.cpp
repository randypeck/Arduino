// MODE_DIAL.CPP Rev: 02/19/24.  NEEDS TESTING AND MAYBE TWEAKING ESPECIALLY TOWARDS THE END OF THIS CODE *******************************************************
// Part of O_MAS.
// A set of functions to read the control panel mode dial controls, and update corresponding LEDs.
// 02/19/24: Removed support for POV mode; just ignore it if operator tries to select that mode.

#include "Mode_Dial.h"

Mode_Dial::Mode_Dial() {
  // no constructor tasks.
  return;
}

void Mode_Dial::begin(byte* t_mode, byte* t_state) {
  *t_mode = MODE_MANUAL;
  *t_state = STATE_STOPPED;
  m_registered = false;
  m_LEDsOn = false;    // Toggle this variable to know when blinking LEDs should be on versus off.
  paintModeLEDs(*t_mode, *t_state);  // The LEDs will all initially be set to dark.
  return;
}

bool Mode_Dial::startButtonIsPressed(byte* t_mode, byte* t_state) {
  // Rev: 03/28/23.  COMPLETE BUT NEEDS TO BE TESTED.
  // Check if the operator has just pressed START BUTTON *and* it is legal to do so.  Paint the MODE LEDs regardless.
  // If it's legal to start the new mode, then the START BUTTON LED will already be illuminated.
  // The calling program never needs to check the MODE DIAL or if it's legal to press the START BUTTON; that's done here.
  // To recognize a START BUTTON push, MODE DIAL must be turned to a new mode that is legal to start based on current MODE/STATE.
  // If a new mode is started and it's MANUAL or REGISTER, then it will also reset m_registered = false.
  // When a new mode is started, the new STATE will always be RUNNING, except PARK mode will become STOPPING because it doesn't
  // make any sense to be RUNNING in PARK mode (otherwise you wouldn't be in PARK mode!)
  // THIS FUNCTION WILL RETURN TRUE AND UPDATE MODE AND STATE POINTERS if (and only if) all conditions are met.
  
  // * If t_state == RUNNING, t_mode == ANY, it is not possible to Start any new mode.

  // * If t_state == STOPPING, t_mode == AUTO, and MODE DIAL is pointing to PARK (m_registered will always already be true):
  //   -> AUTO MODE LED will be blinking, PARK MODE LED will be lit, and START BUTTON LED will be lit.  Ok to start PARK mode.
  // Note that if PARK is STOPPING, the START BUTTON will not be recognized.  No other mode supports STOPPING.

  // * If t_state == STOPPED, t_mode == ANY, and MODE DIAL is pointing to MANUAL/REGISTER/P.O.V:
  //   -> Corresponding MODE DIAL LED will be lit and START BUTTON LED will be lit.  Ok to start that mode.
  // * If t_state == STOPPED, t_mode == ANY, MODE DIAL is pointing to AUTO/PARK, and m_registered == true:
  //   -> Corresponding MODE DIAL LED will be lit and START BUTTON LED will be lit.  Ok to start that mode.
  // Any other STOPPED combinations will not be recognized.

  // First paint the mode control LEDs for the incoming mode/state.  We'll repaint before we exit if they need to change.
  // We do this just to keep the LEDs updated, esp. if we are in AUTO STOPPING or PARK STOPPING, where an LED will be blinking.
  paintModeLEDs(*t_mode, *t_state);

  // If the START BUTTON isn't even being pressed, or the dial is turned to an invalid position, just bail.
  if (!modeButtonStartIsPressed() || (modeDialPosition() == MODE_UNDEFINED)) {
    return false;
  }
  // Okay, the operator is pressing the START BUTTON, and the dial is turned to one of five actual mode positions.

  if (*t_state == STATE_RUNNING) {  // Doesn't matter what mode we're in, can't Start if we're already Running
    // NOTE: This block of code is unnecessary as we'll just fall through the next tests and return false at the end.
    // But including the block here just helps make things more clear.
    return false;
  }

  if (*t_state == STATE_STOPPING) {  // Can only start PARK (STOPPING) from AUTO STOPPING.
    if ((*t_mode == MODE_AUTO) && (modeDialPosition() == MODE_PARK)) {  // Transition from AUTO STOPPING to PARK STOPPING
      // m_registered must already be true in order to get this far; no need to check or set again.
      *t_mode = modeDialPosition();  // MODE_PARK
      *t_state = STATE_STOPPING;
      paintModeLEDs(*t_mode, *t_state);
      chirp();
      return true;
    }
  }

  if (*t_state == STATE_STOPPED) {
    if ((modeDialPosition() == MODE_MANUAL) || (modeDialPosition() == MODE_REGISTER)) {
      *t_mode = modeDialPosition();  // MODE_MANUAL or MODE_REGISTER
      *t_state = STATE_RUNNING;
      m_registered = false;  // Reset registered status because this mode will lose registration
      paintModeLEDs(*t_mode, *t_state);
      chirp();
      return true;
    }
  }

  if (*t_state == STATE_STOPPED) {
    if ((modeDialPosition() == MODE_AUTO) || (modeDialPosition() == MODE_PARK)) {
      if (m_registered == true) {
        *t_mode = modeDialPosition();
        *t_state = STATE_RUNNING;
        paintModeLEDs(*t_mode, *t_state);
        chirp();
        return true;
      }
    }
  }

  // If we fall though to here, we can't start for whatever reason.  We've already painted so just bail.
  return false;
}

bool Mode_Dial::stopButtonIsPressed(const byte t_mode, byte* t_state) {
  // Rev: 03/28/23.  COMPLETE BUT NEEDS TO BE TESTED.
  // Notice that t_mode is passed as a const (not a pointer) and t_state is passed as a pointer as it may be modified.
  // Check if the operator has just pressed STOP BUTTON *and* it is legal to do so.  Paint the MODE LEDs regardless.
  // If it's legal to press the STOP BUTTON from the current mode, then the STOP BUTTON LED will already be illuminated.
  // The calling program never needs to check the MODE DIAL or if it's legal to press the STOP BUTTON; that's done here.
  // It makes no difference how the MODE DIAL is set; that's irrelevant.  We can or can't STOP based on the current MODE and STATE.
  // * Current STATE must be RUNNING for any mode in order to press the STOP BUTTON:
  //   MANUAL mode RUNNING will transition to STOPPED.
  //     The only other possible state for MANUAL is already STOPPED in which case pressing the STOP BUTTON is moot.
  //   REGISTER mode RUNNING cannot be stopped, and there is no REGISTER mode STOPPING.
  //   AUTO mode RUNNING *can* be STOPPED and will transition to AUTO mode STOPPING.  AUTO mode STOPPING cannot be STOPPED.
  //   PARK mode RUNNING never happens; it will be PARK mode STOPPING.  PARK mode STOPPING cannot be STOPPED.
  // THIS FUNCTION WILL RETURN TRUE AND UPDATE MODE AND STATE POINTERS if (and only if) all conditions are met.

  // * If t_state == RUNNING, t_mode == MANUAL:
  //   -> Corresponding MODE LED will be lit and STOP BUTTON LED will be lit.  Ok to stop.  New mode = STOPPED.
  // * If t_state == RUNNING, t_mode == REGISTER:
  //   -> Corresponding MODE LED will be lit but STOP BUTTON LED will be dark.  May not stop.
  // * If t_state == RUNNING, t_mode == AUTO:
  //   -> Corresponding MODE LED will be lit and STOP BUTTON LED will be lit.  Ok to stop.  New mode = STOPPING.
  // Note: t_state RUNNING is not possible when t_mode == PARK.

  // * If t_state == STOPPING, t_mode == AUTO or PARK:
  //   -> Corresponding MODE LED will be blinking but STOP BUTTON LED will be dark.  May not stop.

  // * If t_state == STOPPED, any mode, STOP BUTTON LED will be dark and may not stop (already stopped!)

  // First paint the mode control LEDs for the incoming mode/state.  We'll repaint before we exit if they need to change.
  // We do this just to keep the LEDs updated, esp. if we are in AUTO STOPPING or PARK STOPPING, where an LED will be blinking.
  paintModeLEDs(t_mode, *t_state);

  // If the STOP BUTTON isn't being pressed, or we are in REGISTER mode, just bail.
  if ((!modeButtonStopIsPressed()) || (t_mode == MODE_REGISTER)) {
    return false;
  }
  // Okay, the operator is pressing the STOP BUTTON, and we are *not* in REGISTER mode.

  if ((*t_state == STATE_RUNNING) && (t_mode == MODE_MANUAL)) {
    *t_state = STATE_STOPPED;
    paintModeLEDs(t_mode, *t_state);
    chirp();
    return true;
  }

  if ((*t_state == STATE_RUNNING) && (t_mode == MODE_AUTO)) {
    *t_state = STATE_STOPPING;  // STOPPING, not STOPPED
    paintModeLEDs(t_mode, *t_state);
    chirp();
    return true;
  }

  // If we fall though to here, we can't stop for whatever reason.  We've already painted so just bail.
  return false;
}

void Mode_Dial::setStateToSTOPPED(const byte t_mode, byte* t_state) {
  // Rev: 03/28/23.  COMPLETE BUT NEEDS TO BE TESTED.
  // Set current state to STOPPED unconditionally.  Caller is responsible for ensuring to only call when it's appropriate!
  // Also paints the MODE LEDs.
  // This function is necessary for modes that complete automatically, and *not* when the operator presses the STOP BUTTON.
  // The calling module will call this function to change the current state to STOPPED in three scenarios:
  // * When REGISTER mode completes RUNNING state organically; thus all trains are registered.  It wll also set m_registered true.
  // * When AUTO or PARK mode completes STOPPING state organically; thus all trains are stopped.
  if (t_mode == MODE_REGISTER) {
    m_registered = true;  // Tracked internally to this class
  }
  *t_state = STATE_STOPPED;
  paintModeLEDs(t_mode, *t_state);
  return;
}

void Mode_Dial::paintModeLEDs(const byte t_mode, const byte t_state) {
  // Rev: 03/28/23.  COMPLETE BUT NEEDS TO BE TESTED.
  // Turns the seven LEDs on/off, as appropriate, given current Mode, State, Dial position, and whether or not all trains are
  // registered and stopped.  Registration status is tracked in our internal private bool variable m_registered.
  // This function should be called any time a MODE or STATE changes, and whenever operator turns the MODE dial, and at least every
  // 500ms when in AUTO STOPPING or PARK STOPPING in order to support blinking AUTO/PARK MODE LEDs.

  // * If t_state == RUNNING, regardless of MODE DIAL position:
  //   -> If t_mode == MANUAL or AUTO:
  //      -> Current mode's green MODE LED will be lit, STOP BUTTON LED will be lit.
  //   -> if t_mode == REGISTER:
  //      -> Current mode's green MODE LED will be lit, STOP BUTTON LED will be dark.  Can't STOP REGISTER mode.
  // Note that PARK mode does not have a RUNNING state so can be ignored.

  // * If t_state == STOPPING, t_mode = AUTO, and MODE DIAL is pointing to PARK (m_registered will always be true):
  //   -> AUTO mode's green MODE LED will be blinking, START BUTTON LED will be lit (note: Park mode LED remains dark.)
  // * If t_state == STOPPING, t_mode = AUTO, and MODE DIAL is *not* pointing to PARK:
  //   -> AUTO mode's green MODE LED will be blinking, all other LEDs will be dark.
  // * If t_state == STOPPING, t_mode = PARK (any MODE DIAL position):
  //   -> PARK mode's green MODE LED will be blinking, all other LEDs will be dark.

  // * If t_state == STOPPED, and MODE DIAL is turned to MANUAL or REGISTER:
  //   -> START BUTTON LED will be lit, all others (incl. all MODE LEDs) are dark.
  // * If t_state == STOPPED, and MODE DIAL is turned to AUTO or PARK, *and* m_registered == true:
  //   -> START BUTTON LED will be lit, all others (incl. all MODE LEDs) are dark.
  // * If t_state == STOPPED, and MODE DIAL is turned to AUTO or PARK, *and* m_registered == false:
  //   -> All seven LEDs will be dark.

  // We could just turn off all LEDs and then turn on whichever one(s) should be lit, but this could cause flickering, and more
  // importantly it wastes a lot of time with (slow) digitalWrites.  So instead, we'll use a private variable for each LED to
  // remember how they're set upon entry into this routine.  Then we'll decide which LEDs should be changed from off to on, or from
  // on to off, if any.  We'll write those changed LEDs only, if any, and also update the status variables with the new status.

  // We'll start by defaulting all LED status to off, and individually override with on or blink as appropriate.
  m_LEDManualNew   = LED_DARK;
  m_LEDRegisterNew = LED_DARK;
  m_LEDAutoNew     = LED_DARK;
  m_LEDParkNew     = LED_DARK;
  m_LEDStartNew    = LED_DARK;
  m_LEDStopNew     = LED_DARK;

  if (t_state == STATE_RUNNING) {
    // Illuminate the Running mode, and illuminate Stop if applicable.  Can't be Running in Park, so will always be off.
    if (t_mode == MODE_MANUAL) {
      m_LEDManualNew   = LED_SOLID;
      m_LEDStopNew     = LED_SOLID;  // Set STOP LED on
    } else if (t_mode == MODE_REGISTER) {
      m_LEDRegisterNew = LED_SOLID;
      m_LEDStopNew     = LED_DARK;   // We won't allow STOP LED on when Running in Registration mode
    } else if (t_mode == MODE_AUTO) {
      m_LEDAutoNew     = LED_SOLID;
      m_LEDStopNew     = LED_SOLID;  // Set STOP LED on
    }  // Note that MODE_PARK does not have a RUNNING state

  }

  if (t_state == STATE_STOPPING) {  // Only possible with AUTO and PARK modes
    if (t_mode == MODE_AUTO) {
      m_LEDAutoNew     = LED_BLINKING;
      if (modeDialPosition() == MODE_PARK) {  // When STOPPING AUTO, we allow operator to transition to STOPPING PARK.
        m_LEDStartNew     = LED_SOLID;  // Set START LED on when dial turned to Park
      }  // Else when STOPPING AUTO, and Mode Dial is turned to anything *except* Park, Start button remains dark.
    } else if (t_mode == MODE_PARK) {
      m_LEDParkNew     = LED_BLINKING;
    }
  }

  if (t_state == STATE_STOPPED) {
    if ((modeDialPosition() == MODE_MANUAL) || (modeDialPosition() == MODE_REGISTER)) {
      m_LEDStartNew     = LED_SOLID;  // Set START LED on when dial turned to Manual or Register
    } else if ((modeDialPosition() == MODE_AUTO) || (modeDialPosition() == MODE_PARK)) {
      if (m_registered == true) {  // If we're Registered, okay to Start Auto or Park
        m_LEDStartNew     = LED_SOLID;  // Set START LED on when dial turned to Auto or Park *and* we're Registered
      }  // Else we can't Start Auto or Park mode if not Registered so leave default off
    }
  }

  // Before we paint the LEDs, we need to check if either of our possibly-blinking LEDs should be solid or dark.
  if ((millis() - m_LEDFlashProcessed) > LED_FLASH_MS) {  // If enough time passed to flip the state between on/off
    m_LEDFlashProcessed = millis();   // Reset flash timer
    m_LEDsOn = !m_LEDsOn;             // Tells us generically if a blinking LED should be on or off
  }
  if (m_LEDAutoNew == LED_BLINKING) {
    if (m_LEDsOn) {
      m_LEDAutoNew = LED_SOLID;
    } else {
      m_LEDAutoNew = LED_DARK;
    }
  }
  if (m_LEDParkNew == LED_BLINKING) {
    if (m_LEDsOn) {
      m_LEDParkNew = LED_SOLID;
    } else {
      m_LEDParkNew = LED_DARK;
    }
  }

  // Okay, all m_LEDxxxxNew values have been set for any possible condition.
  // Now we actually send digitalWrites to only the LEDs that changed since the last time through here.
  // NOTE: We digitalWrite "not" the new LED value, because LED_DARK = 0 which is equal to Arduino LOW and turns the LED ON;
  // whereas LED_SOLID = 1, which is equal to Arduino HIGH and turns the LED OFF.
  // This is slightly confusing, but doing so allows us to avoid a bunch of if/then/else blocks.  
  // Alternately I could return a bool from a function that returns the logicl-not of whatever is passed to it.
  // I.e. digitalWrite(PIN_OUT_ROTARY_LED_START, pinValue(m_LEDStartNew));
  if (m_LEDStartNew != m_LEDStartOld) {
    digitalWrite(PIN_OUT_ROTARY_LED_START, !m_LEDStartNew);
  }
  if (m_LEDStopNew != m_LEDStopOld) {
    digitalWrite(PIN_OUT_ROTARY_LED_STOP, !m_LEDStopNew);
  }
  if (m_LEDManualNew != m_LEDManualOld) {
    digitalWrite(PIN_OUT_ROTARY_LED_MANUAL, !m_LEDManualNew);
  }
  if (m_LEDRegisterNew != m_LEDRegisterOld) {
    digitalWrite(PIN_OUT_ROTARY_LED_REGISTER, !m_LEDRegisterNew);
  }
  if (m_LEDAutoNew != m_LEDAutoOld) {
    digitalWrite(PIN_OUT_ROTARY_LED_AUTO, !m_LEDAutoNew);
  }
  if (m_LEDParkNew != m_LEDParkOld) {
    digitalWrite(PIN_OUT_ROTARY_LED_PARK, !m_LEDParkNew);
  }

  // Now update the status of the "old" LEDs
  m_LEDStartOld    = m_LEDStartNew;
  m_LEDStopOld     = m_LEDStopNew;
  m_LEDManualOld   = m_LEDManualNew;
  m_LEDRegisterOld = m_LEDRegisterNew;
  m_LEDAutoOld     = m_LEDAutoNew;
  m_LEDParkOld     = m_LEDParkNew;

  return;
}

byte Mode_Dial::modeDialPosition() {
  // Return the MODE_XXX the dial is set to, regardless of other conditions, 1..5.  I.e. 1 = MODE_MANUAL.
  if (digitalRead(PIN_IN_ROTARY_MANUAL) == LOW) {           // Selector dial point at Manual
   return MODE_MANUAL;
  } else if (digitalRead(PIN_IN_ROTARY_REGISTER) == LOW) {
    return MODE_REGISTER;
  } else if (digitalRead(PIN_IN_ROTARY_AUTO) == LOW) {
    return MODE_AUTO;
  } else if (digitalRead(PIN_IN_ROTARY_PARK) == LOW) {
    return MODE_PARK;
  }  // No support for MODE_POV yet.
  return MODE_UNDEFINED;  // If dial turned to any other position
}

bool Mode_Dial::modeButtonStartIsPressed() {
  // We don't care about bounce, because when pressed, the LED will go out and it won't be pressable again for a while.
  return (digitalRead(PIN_IN_ROTARY_START) == LOW);
}

bool Mode_Dial::modeButtonStopIsPressed() {
  // We don't care about bounce, because when pressed, the LED will go out and it won't be pressable again for a while.
  return (digitalRead(PIN_IN_ROTARY_STOP) == LOW);
}

