#ifndef APP_RELAY_H
#define APP_RELAY_H

#include <jendefs.h>

#include "dbg.h"

#ifdef DEBUG_RELAY
#define TRACE_RELAY TRUE
#else
#define TRACE_RELAY FALSE
#endif

#define RELAY_DBG(...) DBG_vPrintf(TRACE_RELAY, "[Relay] " __VA_ARGS__)

#define PDM_ID_RELAY_STATE 0xA

/* Latching relay support. Active only when the board defines RELAY_SET_DIO / RELAY_RESET_DIO. */
void RELAY_HW_Init(void);
void RELAY_Timers_Init(void);
bool_t RELAY_GetSavedState(void);
void RELAY_SetState(bool_t bOn);
void RELAY_EnableClamp(void);

#endif /* APP_RELAY_H */
