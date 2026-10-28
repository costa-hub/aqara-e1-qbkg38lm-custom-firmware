#include "app_relay.h"

#include "PDM.h"
#include "ZTimer.h"
#include "device_definitions.h"
#include "fsl_gpio.h"
#include "fsl_iocon.h"

#if defined(RELAY_SET_DIO) && defined(RELAY_RESET_DIO)

/* The coil "+" rail (1000 uF) is charged through a high-side switch enabled by RELAY_SUPPLY_DIO. Coil pulses start
 * only once the rail had time to charge. */
typedef enum {
    RELAY_PHASE_SUPPLY_OFF,  // waiting to enable the rail supply after boot
    RELAY_PHASE_CHARGING,    // rail supply on, capacitors charging
    RELAY_PHASE_READY,
    RELAY_PHASE_PULSE,  // one coil energized
} RelayPhase_t;

static void RELAY_TimerCallback(void* pvParam);
static void RELAY_ApplyRequested(void);
static void RELAY_StartPulse(bool_t bOn);

static uint8_t u8RelayTimerID;
static RelayPhase_t ePhase = RELAY_PHASE_SUPPLY_OFF;
/* State the relay was last driven to, and the state requested while the relay was busy */
static bool_t bDrivenState = FALSE;
static bool_t bRequestedState = FALSE;

static void RELAY_ConfigurePin(uint32_t u32Pin) {
    gpio_pin_config_t relay_pin_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 0U,
    };
    IOCON_PinMuxSet(IOCON, 0, u32Pin, IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN);
    GPIO_PinInit(GPIO, 0, u32Pin, &relay_pin_config);
}

/* Coil drivers and the rail supply off. The relay is latching, so it keeps its contacts across a reboot. */
void RELAY_HW_Init(void) {
    RELAY_DBG("Configuring relay: set=%d reset=%d supply=%d\n", RELAY_SET_DIO, RELAY_RESET_DIO, RELAY_SUPPLY_DIO);
    RELAY_ConfigurePin(RELAY_SET_DIO);
    RELAY_ConfigurePin(RELAY_RESET_DIO);
    RELAY_ConfigurePin(RELAY_SUPPLY_DIO);
}

void RELAY_Timers_Init(void) {
    ZTIMER_eOpen(&u8RelayTimerID, RELAY_TimerCallback, NULL, ZTIMER_FLAG_PREVENT_SLEEP);
    bDrivenState = RELAY_GetSavedState();
    bRequestedState = bDrivenState;
    // Enable the rail supply a little after boot so charging the capacitors does not load the start-up
    ePhase = RELAY_PHASE_SUPPLY_OFF;
    ZTIMER_eStart(u8RelayTimerID, ZTIMER_TIME_MSEC(RELAY_SUPPLY_DELAY_MSEC));
}

bool_t RELAY_GetSavedState(void) {
    bool_t bState = FALSE;
    uint16 u16ByteRead;
    PDM_eReadDataFromRecord(PDM_ID_RELAY_STATE, &bState, sizeof(bool_t), &u16ByteRead);
    return bState;
}

/* Applied at once when the relay is ready, otherwise as soon as the rail is charged or the running pulse ends */
void RELAY_SetState(bool_t bOn) {
    RELAY_DBG("Set state %d requested (phase %d)\n", bOn, ePhase);
    bRequestedState = bOn;
    if (ePhase == RELAY_PHASE_READY) {
        RELAY_ApplyRequested();
    }
}

static void RELAY_ApplyRequested(void) {
    if (bRequestedState != bDrivenState) {
        RELAY_StartPulse(bRequestedState);
    }
}

/* Only one coil is energized at a time: a new pulse starts after the previous one ended */
static void RELAY_StartPulse(bool_t bOn) {
    ePhase = RELAY_PHASE_PULSE;
    bDrivenState = bOn;
    GPIO_PinWrite(GPIO, 0, bOn ? RELAY_SET_DIO : RELAY_RESET_DIO, 1U);
    ZTIMER_eStop(u8RelayTimerID);
    ZTIMER_eStart(u8RelayTimerID, ZTIMER_TIME_MSEC(RELAY_PULSE_MSEC));
    PDM_eSaveRecordData(PDM_ID_RELAY_STATE, &bDrivenState, sizeof(bool_t));
}

static void RELAY_TimerCallback(void* pvParam) {
    switch (ePhase) {
        case RELAY_PHASE_SUPPLY_OFF:
            RELAY_DBG("Rail supply on\n");
            GPIO_PinWrite(GPIO, 0, RELAY_SUPPLY_DIO, 1U);
            ePhase = RELAY_PHASE_CHARGING;
            ZTIMER_eStart(u8RelayTimerID, ZTIMER_TIME_MSEC(RELAY_CHARGE_MSEC));
            break;

        case RELAY_PHASE_PULSE:
            GPIO_PinWrite(GPIO, 0, RELAY_SET_DIO, 0U);
            GPIO_PinWrite(GPIO, 0, RELAY_RESET_DIO, 0U);
            /* fall through */
        case RELAY_PHASE_CHARGING:
        default:
            ePhase = RELAY_PHASE_READY;
            RELAY_DBG("Ready, state %d requested %d\n", bDrivenState, bRequestedState);
            RELAY_ApplyRequested();
            break;
    }
}

/* Keeps both coil drivers low while sleeping so no coil stays energized. */
void RELAY_EnableClamp(void) {
    IOCON_PinMuxSet(IOCON, 0, RELAY_SET_DIO, IOCON_FUNC0 | IOCON_MODE_PULLDOWN | IOCON_IO_CLAMPING_NORMAL_MFIO);
    IOCON_PinMuxSet(IOCON, 0, RELAY_RESET_DIO, IOCON_FUNC0 | IOCON_MODE_PULLDOWN | IOCON_IO_CLAMPING_NORMAL_MFIO);
}

#else

void RELAY_HW_Init(void) {}
void RELAY_Timers_Init(void) {}
bool_t RELAY_GetSavedState(void) { return FALSE; }
void RELAY_SetState(bool_t bOn) {}
void RELAY_EnableClamp(void) {}

#endif
