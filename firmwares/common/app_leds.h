#ifndef APP_LEDS_H
#define APP_LEDS_H

#include <jendefs.h>

#include "dbg.h"

#ifdef DEBUG_LEDS
#define TRACE_LEDS TRUE
#else
#define TRACE_LEDS FALSE
#endif

#define LEDS_DBG(...) DBG_vPrintf(TRACE_LEDS, "[LEDS] " __VA_ARGS__)

#define LEDS_BLINK_INTERVAL ZTIMER_TIME_MSEC(100)
#define LEDS_CONTINOUS_BLINK_INTERVAL ZTIMER_TIME_MSEC(150)
#define LEDS_JOINED_INTERVAL ZTIMER_TIME_MSEC(1500)
#define LEDS_SELF_TEST_USEC (300000U)

typedef struct {
    const uint32_t u32DioMask;
} Led_t;

typedef struct {
    const Led_t* psLed;
    uint8_t u8TimerID;
    uint8_t u8IsOn;
} LedWithState_t;

extern const size_t g_numLedsPins;
extern const uint8_t g_asLedsPins[];
extern const size_t g_numLeds;
extern const Led_t* const g_asLeds[];
extern const Led_t g_sNetworkSetupLed;

extern LedWithState_t g_asLedsStates[];

void LEDS_Hardware_Init(void);
void LEDS_Timers_Init(void);
void LEDS_Blink(LedWithState_t* psLedWithState);
void LEDS_TurnOff(LedWithState_t* psLedWithState);
/* Steady level of a button LED between blinks */
void LEDS_SetIndicator(const Led_t* psLed, bool_t bOn);
void LEDS_BlinkDuringNetworkSetup_Start(void* pvParam);
void LEDS_BlinkDuringNetworkSetup_Stop(void);
void LEDS_ButtonBlinkCallback(void* ctx);
void LEDS_IndicateNetworkJoined(void);
void LEDS_EnableClamp(void);

#endif /* APP_LEDS_H */
