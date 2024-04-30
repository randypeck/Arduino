// MODE_DIAL.H Rev: 03/29/23.  ALL CODE WRITTEN BUT NOT TESTED.
// Part of O_MAS.
// 3/29/23: TO DO: Review/re-write/delete all code everywhere except MODE_DIAL.CPP that explains mode dial/LED/start+stop logic,
// so it's all consistent with mode_dial.cpp.
// 03/26/23: Allow change from AUTO+RUNNING to PARK+RUNNING without AUTO+STOPPING or AUTO+STOPPED first.
// However it is NOT possible to change from AUTO+STOPPING or PARK+STOPPING to any other mode -- operator must either wait until
// AUTO or PARK mode stops organically, or they must press an Emergency Stop button.

// This class includes functions to read the control panel MODE DIAL, START and STOP BUTTONS, and illuminate the MODE LEDs.
// The functions include logic to ignore inappropriate START and STOP BUTTON presses.
// The functions generally update MODE and STATE as appropriate.
// The functions generally keep the MODE LEDs up to date including blinking, as long as they're called frequently enough.

// ROTARY SWITCH AND LED Behavior:
//   There is a 5-position ROTARY SWITCH corresponding to each mode: MANUAL, REGISTER, AUTO, PARK, and P.O.V.
//   There are  5 small green MODE LEDs corresponding to each Rotary Switch position (MANUAL, REGISTER, AUTO, PARK, and P.O.V.)
//   There is   1 large green START LED that illuminates the START BUTTON.
//   There is   1 large red   STOP  LED that illuminates the STOP  BUTTON.

// The small green MODE LEDs illuminate SOLID    only when that mode is RUNNING (and not when selecting a mode.)
// The small green MODE LEDs illuminate FLASHING only when AUTO or PARK mode is STOPPING.
// The large green START LED and large red STOP LED illuminate only when it is possible to press the respective STOP/START BUTTONS.
// WHEN STOPPED: The large red STOP LED and all five small green MODE LEDs and are OFF.  Large green START LED illuminates when
//               rotary dial is turned to a mode that can be started at that point.
// NOTE: It is only possible to START AUTO mode or START PARK mode after successful completion of REGISTER mode.
// WHEN RUNNING (START BUTTON pressed): START LED goes dark, MODE LED lights regardless of MODE DIAL position, and STOP LED is lit.
//   EXCEPTION: If in REGISTER mode RUNNING, STOP  LED is off because REGISTER mode does not allow use of the STOP button.
//   EXCEPTION: If in AUTO     mode RUNNING, START LED turns on when MODE DIAL is turned to PARK, to allow operator to START PARK
//              mode RUNNING directly from AUTO mode RUNNING, so it is not necessary to STOP AUTO mode before START PARK mode.
// WHEN STOPPING in AUTO or PARK mode (only): MODE LED blinks while stopping, all other LEDs dark and START/STOP buttons disabled.

#ifndef MODE_DIAL_H
#define MODE_DIAL_H

#include <Train_Consts_Global.h>
#include <Train_Functions.h>
#include <Display_2004.h>

class Mode_Dial {

  public:

    Mode_Dial();  // Constructor must be called above setup() so the object will be global to the module.

    void begin(byte* t_mode, byte* t_state);
    // Power-up in MANUAL MODE, STOPPED STATE, m_registered set to false.

    bool startButtonIsPressed(byte* t_mode, byte* t_state);
    // Check if the operator has just pressed START BUTTON *and* it is legal to do so:
    //   * Current STATE must either be STOPPED for any mode, or can be RUNNING if in AUTO mode and MODE DIAL set to PARK.
    //   * MODE DIAL must be turned to a mode that is legal to start based on previously completed mode.
    //   * START BUTTON is therefore illuminated *only* when it may be pressed by the operator.
    // The calling program never needs to check the MODE DIAL or if it's legal for the operator to press the START BUTTON.
    // THIS FUNCTION WILL RETURN TRUE AND UPDATE MODE AND STATE POINTERS if (and only if) all conditions are met.
    // If a new mode is started and it's MANUAL or REGISTER, then it will also reset m_registered = false.
    // It will also re-paint the MODE DIAL LEDs and START/STOP BUTTON LEDs as appropriate.
    // When a new mode is started, the new State will always be RUNNING.

    bool stopButtonIsPressed(const byte t_mode, byte* t_state);
    // Check if the operator has just pressed STOP BUTTON *and* it is legal to do so:
    //   * Current STATE must be RUNNING for Stop to be relevant.  Can't press Stop when STOPPING or STOPPED.
    //   * Mode must be MANUAL or AUTO
    //     * It makes no difference how the MODE DIAL is set; that's irrelevant.
    //     * REGISTER mode can't be STOPPED.
    //     * PARK mode immediately transitions from RUNNING to STOPPING, which can't be STOPPED.
    //   * STOP BUTTON is therefore illuminated *only* when it may be pressed by the operator.
    // Since this function decides when STOP BUTTON may be legally pressed, calling program doesn't need to worry about that.
    // If legal, and depending on Mode, t_state will become either STOPPED (MANUAL), or STOPPING (AUTO.)

    void setStateToSTOPPED(const byte t_mode, byte* t_state);
    // The calling module will call this function to change the current state to STOPPED in a couple of scenarios:
    //   * When REGISTER mode ends normally (goes from RUNNING to STOPPED.)  It will also set m_registered true.
    //   * When AUTO or PARK mode ends normally (goes from STOPPING to STOPPED.)
    // Note that when in MANUAL mode, when the operator presses STOP BUTTON (goes from RUNNING to STOPPED,) then
    // the "stopButtonIsPressed()" function will see it and return the new mode.
    // It will also re-paint the Control Panel LEDs as appropriate.

    void paintModeLEDs(const byte t_mode, const byte t_state);
    // Turns the seven LEDs on/off, as appropriate, given current Mode, State, and whether or not all trains are REGISTERED and
    // STOPPED.  Note that even if trains were REGISTERED, if the operator starts MANUAL or REGISTER modes, then
    // m_registered registration status is reset to false.
    // This function will be called from all of the above class functions, but must also be called from the main program at least
    // every 500ms if we are to support flashing MODE LEDs (when Mode is STOPPING.)
    // Ditto when State is STOPPED and operator is turning MODE DIAL, so we can illuminate the START BUTTON when appropriate.

  private:

    byte modeDialPosition();
    bool modeButtonStartIsPressed();
    bool modeButtonStopIsPressed();

    const byte LED_DARK              =   0;  // HIGH = 1, LOW = 0, so this will be the reverse of what we digitalWrite()
    const byte LED_SOLID             =   1;
    const byte LED_BLINKING          =   2;
    const unsigned long LED_FLASH_MS = 500;  // Blinking LED blink toggle rate (MODE LED when STOPPING AUTO or PARK)

    bool m_registered = false;
    // m_registered is used to determine if it is legal to start AUTO or PARK modes.  Set true when REGISTER mode completes,
    // otherwise false i.e. when we first start, and whenever MANUAL mode is started.

    // Define a set of member variables to keep track of the status of each LED.
    // The "paint" function needs both and "old" and a "new" value (which it sets) for each LED.
    // Possible m_LEDxxx values are LED_DARK and LED_SOLID.  BLINKING is handled by private timers automatically.
    byte m_LEDStartOld    = LED_DARK;  // Upon ENTRY to this function.
    byte m_LEDStartNew    = LED_DARK;  // Upon EXIT from this function.
    byte m_LEDStopOld     = LED_DARK;
    byte m_LEDStopNew     = LED_DARK;
    byte m_LEDManualOld   = LED_DARK;
    byte m_LEDManualNew   = LED_DARK;
    byte m_LEDRegisterOld = LED_DARK;
    byte m_LEDRegisterNew = LED_DARK;
    byte m_LEDAutoOld     = LED_DARK;
    byte m_LEDAutoNew     = LED_DARK;
    byte m_LEDParkOld     = LED_DARK;
    byte m_LEDParkNew     = LED_DARK;

    // We have a timer to track the (potentially) flashing Auto or Park LED to toggle on/off only every 1/2 second or so.
    unsigned long m_LEDFlashProcessed = millis();   // Delay between toggling flashing LEDs on control panel i.e. 1/2 second.
    bool m_LEDsOn = false;    // Toggle this variable to know when blinking LEDs should be on versus off.

};

#endif
