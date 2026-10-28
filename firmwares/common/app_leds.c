#include "app_leds.h"

#include "ZTimer.h"
#include "device_config.h"
#include "fsl_clock.h"
#include "fsl_gpio.h"
#include "fsl_iocon.h"
#include "fsl_wwdt.h"

static void LEDS_BlinkTurnOffCallback(void* pvParam);
static LedWithState_t* LEDS_GetStateFromLed(const Led_t* led);
static void LEDS_BlinkFor(LedWithState_t* psLedWithState, uint32_t u32Interval);
static void LEDS_SelfTest(void);

LedWithState_t sNetworkSetupLedState;
/* Network setup LED blinked on button press while the device is not joined */
static LedWithState_t sNotJoinedLedState;
/* Button LEDs that are lit steadily between blinks (relay state indicator) */
static uint32_t u32IndicatorMask = 0;

void LEDS_Hardware_Init(void) {
    gpio_pin_config_t led_pin_config = {
        .pinDirection = kGPIO_DigitalOutput,
        .outputLogic = 1U,
    };

    for (uint8_t i = 0; i < g_numLedsPins; i++) {
        LEDS_DBG("LED(%u). Configuring Hardware...\n", g_asLedsPins[i]);
        IOCON_PinMuxSet(IOCON, 0, g_asLedsPins[i], IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_DIGITAL_EN);
        GPIO_PinInit(GPIO, 0, g_asLedsPins[i], &led_pin_config);
    }
    LEDS_SelfTest();
}

/* Network setup LED, then button LEDs, each lit briefly at boot to check LED power and pins */
static void LEDS_SelfTest(void) {
    uint32_t u32ButtonLedsMask = 0;
    for (uint8_t i = 0; i < g_numLeds; i++) {
        u32ButtonLedsMask |= g_asLeds[i]->u32DioMask;
    }

    GPIO_PortClear(GPIO, 0, g_sNetworkSetupLed.u32DioMask);
    CLOCK_uDelay(LEDS_SELF_TEST_USEC);
    GPIO_PortSet(GPIO, 0, g_sNetworkSetupLed.u32DioMask);
    WWDT_Refresh(WWDT);

    GPIO_PortClear(GPIO, 0, u32ButtonLedsMask);
    CLOCK_uDelay(LEDS_SELF_TEST_USEC);
    GPIO_PortSet(GPIO, 0, u32ButtonLedsMask);
    WWDT_Refresh(WWDT);
}

void LEDS_Timers_Init(void) {
    LEDS_DBG("Configuring Timers...\n");
    for (uint8_t i = 0; i < g_numLeds; i++) {
        g_asLedsStates[i].psLed = g_asLeds[i];
        g_asLedsStates[i].u8IsOn = FALSE;
        ZTIMER_eOpen(&g_asLedsStates[i].u8TimerID, LEDS_BlinkTurnOffCallback, &g_asLedsStates[i], ZTIMER_FLAG_PREVENT_SLEEP);
        LEDS_DBG("LED(%u). Blink turn off Timer ID - %d\n", g_asLeds[i]->u32DioMask, g_asLedsStates[i].u8TimerID);
    }

    sNotJoinedLedState.psLed = &g_sNetworkSetupLed;
    sNotJoinedLedState.u8IsOn = FALSE;
    ZTIMER_eOpen(&sNotJoinedLedState.u8TimerID, LEDS_BlinkTurnOffCallback, &sNotJoinedLedState, ZTIMER_FLAG_PREVENT_SLEEP);

    sNetworkSetupLedState.psLed = &g_sNetworkSetupLed;
    sNetworkSetupLedState.u8IsOn = FALSE;
    ZTIMER_eOpen(&sNetworkSetupLedState.u8TimerID, LEDS_BlinkDuringNetworkSetup_Start, NULL, ZTIMER_FLAG_PREVENT_SLEEP);
    LEDS_DBG("LEDS_Init finished. Blink during setup Timer ID - %d\n", sNetworkSetupLedState.u8TimerID);
}

void LEDS_Blink(LedWithState_t* psLedWithState) {
    LEDS_BlinkFor(psLedWithState, LEDS_BLINK_INTERVAL);
}

static void LEDS_BlinkFor(LedWithState_t* psLedWithState, uint32_t u32Interval) {
    ZTIMER_teState state = ZTIMER_eGetState(sNetworkSetupLedState.u8TimerID);
    if (state == E_ZTIMER_STATE_RUNNING) {
        LEDS_DBG("Blink during setup in progress. Do nothing...");
        return;
    }

    LEDS_DBG("LED(%u). Blink called\n", psLedWithState->psLed->u32DioMask);
    if (psLedWithState->u8IsOn == FALSE) {
        // An indicator that is lit steadily blinks by going dark for the interval
        if (psLedWithState->psLed->u32DioMask & u32IndicatorMask) {
            GPIO_PortSet(GPIO, 0, psLedWithState->psLed->u32DioMask);
        } else {
            GPIO_PortClear(GPIO, 0, psLedWithState->psLed->u32DioMask);
        }
        psLedWithState->u8IsOn = TRUE;
        LEDS_DBG("LED(%u). Starting turn off timer %d\n", psLedWithState->psLed->u32DioMask, psLedWithState->u8TimerID);
        ZTIMER_teStatus status = ZTIMER_eStart(psLedWithState->u8TimerID, u32Interval);
        LEDS_DBG("LED(%u). Start turn off timer %d status: %d\n", psLedWithState->psLed->u32DioMask, psLedWithState->u8TimerID, status);
    }
}

void LEDS_TurnOff(LedWithState_t* psLedWithState) {
    LEDS_DBG("LED(%u). Turn Off called\n", psLedWithState->psLed->u32DioMask);
    GPIO_PortSet(GPIO, 0, psLedWithState->psLed->u32DioMask);
    // Back to the steady indicator level after a blink
    GPIO_PortClear(GPIO, 0, psLedWithState->psLed->u32DioMask & u32IndicatorMask);
    psLedWithState->u8IsOn = FALSE;
}

void LEDS_SetIndicator(const Led_t* psLed, bool_t bOn) {
    LEDS_DBG("LED(%u). Indicator %d\n", psLed->u32DioMask, bOn);
    if (bOn) {
        u32IndicatorMask |= psLed->u32DioMask;
    } else {
        u32IndicatorMask &= ~psLed->u32DioMask;
    }
    LedWithState_t* psLedWithState = LEDS_GetStateFromLed(psLed);
    if (psLedWithState && psLedWithState->u8IsOn) {
        return;  // a blink is running, LEDS_TurnOff applies the new level
    }
    if (bOn) {
        GPIO_PortClear(GPIO, 0, psLed->u32DioMask);
    } else {
        GPIO_PortSet(GPIO, 0, psLed->u32DioMask);
    }
}

void LEDS_BlinkDuringNetworkSetup_Start(void* pvParam) {
    if (sNetworkSetupLedState.u8IsOn) {
        LEDS_DBG("Blink during setup. Turning OFF\n");
        GPIO_PortSet(GPIO, 0, sNetworkSetupLedState.psLed->u32DioMask);
        sNetworkSetupLedState.u8IsOn = FALSE;
        ZTIMER_eStart(sNetworkSetupLedState.u8TimerID, LEDS_CONTINOUS_BLINK_INTERVAL);
    } else {
        LEDS_DBG("Blink during setup. Turning ON\n");
        GPIO_PortClear(GPIO, 0, sNetworkSetupLedState.psLed->u32DioMask);
        sNetworkSetupLedState.u8IsOn = TRUE;
        ZTIMER_eStart(sNetworkSetupLedState.u8TimerID, LEDS_CONTINOUS_BLINK_INTERVAL);
    }
}

void LEDS_BlinkDuringNetworkSetup_Stop(void) {
    LEDS_DBG("Blink during setup. Stopping...\n");
    GPIO_PortSet(GPIO, 0, sNetworkSetupLedState.psLed->u32DioMask);
    sNetworkSetupLedState.u8IsOn = FALSE;
    ZTIMER_eStop(sNetworkSetupLedState.u8TimerID);
}

void LEDS_ButtonBlinkCallback(void* ctx) {
    Led_t* led = (Led_t*)ctx;
    LedWithState_t* state = LEDS_GetStateFromLed(led);
    if (!state)
        return;

    LEDS_DBG("LED(%u). Button blink callback called...\n", led->u32DioMask);
    if (!device_config.bIsJoined) {
        LEDS_Blink(&sNotJoinedLedState);
        return;
    }
    LEDS_Blink(state);
}

void LEDS_IndicateNetworkJoined(void) {
    for (uint8_t i = 0; i < g_numLeds; i++) {
        LEDS_BlinkFor(&g_asLedsStates[i], LEDS_JOINED_INTERVAL);
    }
}

void LEDS_EnableClamp(void) {
    for (uint8_t i = 0; i < g_numLedsPins; i++) {
        IOCON_PinMuxSet(IOCON, 0, g_asLedsPins[i], IOCON_FUNC0 | IOCON_MODE_INACT | IOCON_IO_CLAMPING_NORMAL_MFIO);
    }
}

static void LEDS_BlinkTurnOffCallback(void* pvParam) {
    LedWithState_t* ledWithState = (LedWithState_t*)pvParam;
    LEDS_DBG("LED(%u). Blink turn off callback called\n", ledWithState->psLed->u32DioMask);
    LEDS_TurnOff(ledWithState);
}

static LedWithState_t* LEDS_GetStateFromLed(const Led_t* led) {
    for (size_t i = 0; i < g_numLeds; i++) {
        if (g_asLedsStates[i].psLed == led)
            return &g_asLedsStates[i];
    }
    return NULL;
}
