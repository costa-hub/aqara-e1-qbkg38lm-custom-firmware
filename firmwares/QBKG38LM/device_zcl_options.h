#ifndef DEVICE_ZCL_OPTIONS_H
#define DEVICE_ZCL_OPTIONS_H

#define CLD_BAS_MODEL_STR "lumi.switch.b1lc04.diy"
#define CLD_BAS_MODEL_ID_SIZE (22)
#define CLD_BAS_POWER_SOURCE E_CLD_BAS_PS_SINGLE_PHASE_MAINS

/* The rocker endpoint hosts the relay: On/Off server instead of the remote's On/Off client */
#define ONOFF_SERVER
/* Default battery reports plus the On/Off state report configured by the coordinator */
#define ZCL_NUMBER_OF_REPORTS 3
/* Accept Configure Reporting / Read Reporting Configuration from the coordinator */
#define ZCL_CONFIGURE_ATTRIBUTE_REPORTING_SERVER_SUPPORTED
#define ZCL_READ_ATTRIBUTE_REPORTING_CONFIGURATION_SERVER_SUPPORTED

#endif
