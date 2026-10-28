#include "app_on_off_ep.h"

#include "Groups.h"
#include "MultistateInputBasic.h"
#include "OnOff.h"
#include "app_basic_ep.h"
#include "app_leds.h"
#include "app_relay.h"
#include "dbg.h"
#include "device_config.h"
#include "device_definitions.h"
#include "zps_gen.h"

static tsZHA_OnOffEndpoint tsOnOffButtonEndpoints[BUTTONS_AMOUNT];

static void OnOff_EP_ConfigureEP(uint8 u8EndPoint, tsZHA_OnOffEndpoint* tsEndpoint);
static void OnOff_EP_Callback(tsZCL_CallBackEvent* psEvent);
#ifdef ONOFF_SERVER
static tsZHA_OnOffEndpoint* OnOff_EP_Find(uint8 u8Endpoint);
static void OnOff_EP_ApplyState(tsZHA_OnOffEndpoint* tsEndpoint);
#endif

void OnOff_EP_Init(void) {
    int i = 0;
    for (i = 0; i < g_numButtons; i++) {
        OnOff_EP_ConfigureEP(g_asButtons[i].u8Endpoint, &tsOnOffButtonEndpoints[i]);
    }
#ifdef ONOFF_SERVER
    OnOff_EP_UpdateIndicators();
#endif
}

static void OnOff_EP_ConfigureEP(uint8 u8Endpoint, tsZHA_OnOffEndpoint* tsEndpoint) {
    ON_OFF_EP_DBG("Registering endpoint: %d\n", u8Endpoint);
    tsEndpoint->sEndPoint.u8EndPointNumber = u8Endpoint;
    tsEndpoint->sEndPoint.u16ManufacturerCode = ZCL_MANUFACTURER_CODE;
    tsEndpoint->sEndPoint.u16ProfileEnum = HA_PROFILE_ID;
    tsEndpoint->sEndPoint.bIsManufacturerSpecificProfile = FALSE;
    tsEndpoint->sEndPoint.u16NumberOfClusters = sizeof(tsZHA_OnOffEndpointClusterInstances) / sizeof(tsZCL_ClusterInstance);
    tsEndpoint->sEndPoint.psClusterInstance = (tsZCL_ClusterInstance*)&tsEndpoint->sClusterInstance;
    tsEndpoint->sEndPoint.bDisableDefaultResponse = ZCL_DISABLE_DEFAULT_RESPONSES;
    tsEndpoint->sEndPoint.pCallBackFunctions = &OnOff_EP_Callback;

#ifdef ONOFF_SERVER
    teZCL_Status eZCL_Status =
        eCLD_OnOffCreateOnOff(&tsEndpoint->sClusterInstance.sOnOffServer, TRUE, &sCLD_OnOff, &tsEndpoint->sOnOffServerCluster,
                              &au8OnOffAttributeControlBits[0], &tsEndpoint->sOnOffServerCustomDataStructure);
    // The latching relay kept its contacts across the reboot: start from the last driven state without pulsing
    tsEndpoint->sOnOffServerCluster.bOnOff = RELAY_GetSavedState();
    // Without power outage memory the relay is opened after power returns, once the supply capacitors are charged
    if (!BASIC_EP_GetConfiguration()->bPowerOutageMemory && tsEndpoint->sOnOffServerCluster.bOnOff) {
        tsEndpoint->sOnOffServerCluster.bOnOff = FALSE;
        RELAY_SetState(FALSE);  // applied once the coil rail is charged
    }
#else
    teZCL_Status eZCL_Status = eCLD_OnOffCreateOnOff(&tsEndpoint->sClusterInstance.sOnOffClient, FALSE, &sCLD_OnOffClient,
                                                     &tsEndpoint->sOnOffClientCluster, &au8OnOffClientAttributeControlBits[0], NULL);
#endif
    if (eZCL_Status != E_ZCL_SUCCESS) {
        ON_OFF_EP_DBG("eCLD_OnOffCreateOnOff failed with status: %d\n", eZCL_Status);
    }

    eZCL_Status = eCLD_MultistateInputBasicCreateMultistateInputBasic(
        &tsEndpoint->sClusterInstance.sMultistateInputServer, TRUE, &sCLD_MultistateInputBasic, &tsEndpoint->sMultistateInputServerCluster,
        &au8MultistateInputBasicAttributeControlBits[0]);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        ON_OFF_EP_DBG("eCLD_MultistateInputBasicCreateMultistateInputBasic failed with status: %d\n", eZCL_Status);
    }

    eZCL_Status =
        eCLD_GroupsCreateGroups(&tsEndpoint->sClusterInstance.sGroupsClient, FALSE, &sCLD_Groups, &tsEndpoint->sGroupsClientCluster,
                                &au8GroupsAttributeControlBits[0], &tsEndpoint->sGroupsClientCustomDataStructure, &tsEndpoint->sEndPoint);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        ON_OFF_EP_DBG("eCLD_GroupsCreateGroups failed with status: %d\n", eZCL_Status);
    }

    eZCL_Status = eZCL_Register(&tsEndpoint->sEndPoint);
    if (eZCL_Status != E_ZCL_SUCCESS) {
        ON_OFF_EP_DBG("eZCL_Register failed with status: %d\n", eZCL_Status);
    }
}

#ifdef ONOFF_SERVER
void OnOff_EP_Toggle(uint8 u8Endpoint) {
    tsZHA_OnOffEndpoint* tsEndpoint = OnOff_EP_Find(u8Endpoint);
    if (!tsEndpoint)
        return;

    tsEndpoint->sOnOffServerCluster.bOnOff = !tsEndpoint->sOnOffServerCluster.bOnOff;
    ON_OFF_EP_DBG("Local toggle EP=%d state=%d\n", u8Endpoint, tsEndpoint->sOnOffServerCluster.bOnOff);
    OnOff_EP_ApplyState(tsEndpoint);
}

static tsZHA_OnOffEndpoint* OnOff_EP_Find(uint8 u8Endpoint) {
    for (int i = 0; i < g_numButtons; i++) {
        if (tsOnOffButtonEndpoints[i].sEndPoint.u8EndPointNumber == u8Endpoint) {
            return &tsOnOffButtonEndpoints[i];
        }
    }
    return NULL;
}

void OnOff_EP_UpdateIndicators(void) {
    const teCLD_LedMode eLedMode = BASIC_EP_GetConfiguration()->eLedMode;
    for (int i = 0; i < g_numButtons; i++) {
        if (!g_asButtons[i].pvLedConfig)
            continue;
        const bool_t bRelayOn = tsOnOffButtonEndpoints[i].sOnOffServerCluster.bOnOff;
        bool_t bLedOn;
        switch (eLedMode) {
            case E_CLD_LED_MODE_ON_WHEN_ON:
                bLedOn = bRelayOn;
                break;
            case E_CLD_LED_MODE_ALWAYS:
                bLedOn = TRUE;
                break;
            case E_CLD_LED_MODE_NEVER:
                bLedOn = FALSE;
                break;
            case E_CLD_LED_MODE_ON_WHEN_OFF:
            default:
                bLedOn = !bRelayOn;
                break;
        }
        LEDS_SetIndicator((const Led_t*)g_asButtons[i].pvLedConfig, bLedOn);
    }
}

/* Drives the relay to the On/Off attribute and reports it to the coordinator */
static void OnOff_EP_ApplyState(tsZHA_OnOffEndpoint* tsEndpoint) {
    RELAY_SetState(tsEndpoint->sOnOffServerCluster.bOnOff);
    OnOff_EP_UpdateIndicators();
    if (!device_config.bIsJoined)
        return;

    tsZCL_Address addr;
    addr.uAddress.u16DestinationAddress = 0x0000;
    addr.eAddressMode = E_ZCL_AM_SHORT_NO_ACK;
    PDUM_thAPduInstance myPDUM_thAPduInstance = hZCL_AllocateAPduInstance();
    teZCL_Status eStatus = eZCL_ReportAttribute(&addr, GENERAL_CLUSTER_ID_ONOFF, E_CLD_ONOFF_ATTR_ID_ONOFF,
                                                tsEndpoint->sEndPoint.u8EndPointNumber, 1, myPDUM_thAPduInstance);
    PDUM_eAPduFreeAPduInstance(myPDUM_thAPduInstance);
    ON_OFF_EP_DBG("On/Off report status: %02x\n", eStatus);
}
#endif

static void OnOff_EP_Callback(tsZCL_CallBackEvent* psEvent) {
    switch (psEvent->eEventType) {
#ifdef ONOFF_SERVER
        case E_ZCL_CBET_CLUSTER_UPDATE:
            if (psEvent->psClusterInstance->psClusterDefinition->u16ClusterEnum == GENERAL_CLUSTER_ID_ONOFF) {
                tsZHA_OnOffEndpoint* tsEndpoint = OnOff_EP_Find(psEvent->u8EndPoint);
                if (tsEndpoint) {
                    ON_OFF_EP_DBG("On/Off command EP=%d state=%d\n", psEvent->u8EndPoint, tsEndpoint->sOnOffServerCluster.bOnOff);
                    OnOff_EP_ApplyState(tsEndpoint);
                }
            }
            break;
#endif
        default:
            ON_OFF_EP_DBG("Unhandled event type (%d) in OnOff_EP_Callback\n", psEvent->eEventType);
            break;
    }
}
