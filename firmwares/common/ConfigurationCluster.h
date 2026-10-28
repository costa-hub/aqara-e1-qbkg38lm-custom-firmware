/*****************************************************************************
 * Custom configuration cluster
 ****************************************************************************/
#ifndef CONFIGURATION_CLUSTER_H
#define CONFIGURATION_CLUSTER_H

#include "dbg.h"
#include "zcl.h"
#include "zcl_options.h"

#ifdef DEBUG_CONFIGURATION_CLUSTER
#define TRACE_CONFIGURATION_CLUSTER TRUE
#else
#define TRACE_CONFIGURATION_CLUSTER FALSE
#endif

#define CONFIG_CLUSTER_DBG(...) DBG_vPrintf(TRACE_CONFIGURATION_CLUSTER, "[Configuration Cluster] " __VA_ARGS__)

#define GENERAL_CLUSTER_ID_CONFIGURATION 0xfc00
#define PDM_ID_CONFIGURATION_CLUSTER 0x9

typedef enum {
    E_CLD_BUTTON_MODE_ATTR_ID_MODE_TYPE = 0x0000,
    E_CLD_PREVENT_RESET_ATTR_ID_MODE_TYPE = 0x0001,
    E_CLD_RELAY_DECOUPLED_ATTR_ID = 0x0002,
    E_CLD_LED_MODE_ATTR_ID = 0x0003,
    E_CLD_POWER_OUTAGE_MEMORY_ATTR_ID = 0x0004,
} teCLD_Configuration_Attr;

/* When the button LED is lit steadily */
typedef enum {
    E_CLD_LED_MODE_ON_WHEN_OFF = 0,  // relay off (find the switch in the dark)
    E_CLD_LED_MODE_ON_WHEN_ON = 1,
    E_CLD_LED_MODE_ALWAYS = 2,
    E_CLD_LED_MODE_NEVER = 3,
} teCLD_LedMode;

// Must match ButtonMode_t in app_buttons.h
typedef enum {
    E_CLD_BUTTON_MODE_TOGGLE = 0,
    E_CLD_BUTTON_MODE_MOMENTARY_ON_OFF = 1,
    E_CLD_BUTTON_MODE_MULTISTATE_INPUT = 2,
} teCLD_ButtonMode;

typedef void (*ConfigClusterModeChangedCallback)(teCLD_ButtonMode eMode);

typedef struct {
    zenum8 eButtonMode;
    zbool bPreventReset;
    zbool bRelayDecoupled;     // TRUE: the button does not switch the relay, only reports actions
    zenum8 eLedMode;           // teCLD_LedMode
    zbool bPowerOutageMemory;  // FALSE: the relay is opened after power returns
} tsCLD_Configuration;

/* Called after any configuration attribute was written */
void APP_ConfigurationChanged(void);

teZCL_Status eCLD_ConfigurationCreateConfiguration(tsZCL_ClusterInstance* psClusterInstance, bool_t bIsServer,
                                                   tsZCL_ClusterDefinition* psClusterDefinition, void* pvEndPointSharedStructPtr,
                                                   uint8* pu8AttributeControlBits);

extern tsZCL_ClusterDefinition sCLD_Configuration;
extern uint8 au8ConfigurationAttributeControlBits[];
extern const tsZCL_AttributeDefinition asCLD_ConfigurationClusterAttributeDefinitions[];
extern const ConfigClusterModeChangedCallback g_cbZCLModeChanged;

void CONFIGURATION_CLUSTER_HandleAttrsRange(tsZCL_CallBackEvent* psEvent);
void CONFIGURATION_CLUSTER_HandleAttrsWrite(tsZCL_CallBackEvent* psEvent, tsCLD_Configuration* sConfigCluster);
void CONFIGURATION_CLUSTER_ResetPDMRecord(void);

#endif /* CONFIGURATION_CLUSTER_H */
