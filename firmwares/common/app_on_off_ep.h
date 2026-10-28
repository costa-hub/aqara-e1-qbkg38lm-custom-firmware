#ifndef APP_ON_OFF_EP_H
#define APP_ON_OFF_EP_H

#include "Groups.h"
#include "MultistateInputBasic.h"
#include "OnOff.h"
#include "dbg.h"
#include "zcl.h"

#ifdef DEBUG_ON_OFF_EP
#define TRACE_ON_OFF_EP TRUE
#else
#define TRACE_ON_OFF_EP FALSE
#endif

#define ON_OFF_EP_DBG(...) DBG_vPrintf(TRACE_ON_OFF_EP, "[OnOff EP] " __VA_ARGS__)

typedef struct {
#ifdef ONOFF_SERVER
    tsZCL_ClusterInstance sOnOffServer;
#else
    tsZCL_ClusterInstance sOnOffClient;
#endif
    tsZCL_ClusterInstance sGroupsClient;
    tsZCL_ClusterInstance sMultistateInputServer;
} tsZHA_OnOffEndpointClusterInstances __attribute__((aligned(4)));

typedef struct {
    tsZCL_EndPointDefinition sEndPoint;
    tsZHA_OnOffEndpointClusterInstances sClusterInstance;

#ifdef ONOFF_SERVER
    tsCLD_OnOff sOnOffServerCluster;
    tsCLD_OnOffCustomDataStructure sOnOffServerCustomDataStructure;
#else
    tsCLD_OnOffClient sOnOffClientCluster;
#endif
    tsCLD_MultistateInputBasic sMultistateInputServerCluster;

    tsCLD_Groups sGroupsClientCluster;
    tsCLD_GroupsCustomDataStructure sGroupsClientCustomDataStructure;
} tsZHA_OnOffEndpoint;

void OnOff_EP_Init(void);
#ifdef ONOFF_SERVER
/* Toggles the relay from the local button and reports the new state */
void OnOff_EP_Toggle(uint8 u8Endpoint);
/* Sets the button LEDs from the relay state and the configured LED mode */
void OnOff_EP_UpdateIndicators(void);
#endif

#endif /* APP_ON_OFF_EP_H */
