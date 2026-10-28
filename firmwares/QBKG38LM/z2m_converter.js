const {Zcl} = require("zigbee-herdsman");
const mExt = require("zigbee-herdsman-converters/lib/modernExtend");

const manufacturerCode = 0x1037;
const manufacturerOptions = {manufacturerCode: manufacturerCode, disableDefaultResponse: true};
const manufacturerClusterAttributes = {
    buttonMode: {ID: 0x0000, type: Zcl.DataType.ENUM8, write: true},
    preventReset: {ID: 0x0001, type: Zcl.DataType.BOOLEAN, write: true},
    relayDecoupled: {ID: 0x0002, type: Zcl.DataType.BOOLEAN, write: true},
    ledMode: {ID: 0x0003, type: Zcl.DataType.ENUM8, write: true},
    powerOutageMemory: {ID: 0x0004, type: Zcl.DataType.BOOLEAN, write: true},
};
const deviceExtend = {
    ManuConfigurationCluster: () => {
        const extend = mExt.deviceAddCustomCluster("ManuConfiguration", {
            name: "ManuConfiguration",
            ID: 0xfc00,
            manufacturerCode: manufacturerCode,
            attributes: manufacturerClusterAttributes,
            commands: {},
            commandsResponse: {},
        });
        extend.configure.push(mExt.setupConfigureForReading("ManuConfiguration", ["buttonMode", "preventReset", "relayDecoupled", "ledMode", "powerOutageMemory"]));
        return extend;
    },
};
const operationModesLookup = {
    control_relay: 0,
    decoupled: 1,
};
const ledModesLookup = {
    on_when_off: 0,
    on_when_on: 1,
    always: 2,
    never: 3,
};
const buttonModesLookup = {
    toggle: 0,
    momentary_on_off: 1,
    multistate: 2,
};

const multiStateActions = {
    toggle: 0,
    momentary_pressed: 1,
    momentary_released: 2,
    single_click: 3,
    double_click: 4,
    triple_click: 5,
    hold: 6,
    release: 7,
};

module.exports = [
    {
        zigbeeModel: ["lumi.switch.b1lc04.diy"],
        model: "QBKG38LM",
        vendor: "Aqara",
        description: "Smart wall switch E1, no neutral, single rocker (custom firmware)",
        // The relay (On/Off server) is on the rocker endpoint 2; endpoint 1 holds Basic and the configuration cluster
        endpoint: (device) => ({default: 2}),
        // Mains powered: the Power Configuration cluster left in the firmware only reports VBAT, so no battery expose
        fromZigbee: [],
        toZigbee: [],
        exposes: [],
        extend: [
            deviceExtend.ManuConfigurationCluster(),
            mExt.onOff({powerOnBehavior: false}),
            mExt.enumLookup({
                name: "operation_mode",
                lookup: operationModesLookup,
                cluster: "ManuConfiguration",
                attribute: "relayDecoupled",
                description: "control_relay: the rocker switches the relay. decoupled: the rocker only sends actions.",
                zigbeeCommandOptions: manufacturerOptions,
                access: "ALL",
                label: "Operation Mode",
            }),
            mExt.enumLookup({
                name: "button_mode",
                lookup: buttonModesLookup,
                cluster: "ManuConfiguration",
                attribute: "buttonMode",
                description: "Which actions the rocker sends. multistate waits for double and triple clicks before single_click.",
                zigbeeCommandOptions: manufacturerOptions,
                access: "ALL",
                label: "Button Mode",
            }),
            mExt.enumLookup({
                name: "led_mode",
                lookup: ledModesLookup,
                cluster: "ManuConfiguration",
                attribute: "ledMode",
                description: "When the blue LED is lit: while the relay is off, while it is on, always or never.",
                zigbeeCommandOptions: manufacturerOptions,
                access: "ALL",
                label: "LED Mode",
            }),
            mExt.binary({
                name: "power_outage_memory",
                valueOn: [true, 1],
                valueOff: [false, 0],
                cluster: "ManuConfiguration",
                attribute: "powerOutageMemory",
                description: "true: after a power outage the relay keeps its state. false: the relay is turned off 3 s after power returns.",
                zigbeeCommandOptions: manufacturerOptions,
                access: "ALL",
                label: "Power Outage Memory",
            }),
            mExt.binary({
                name: "prevent_reset",
                valueOn: ["ON", 1],
                valueOff: ["OFF", 0],
                cluster: "ManuConfiguration",
                attribute: "preventReset",
                description: "Enable to avoid unintended resets and support HOLD functionality. Without it, a 5 s hold resets the device.",
                zigbeeCommandOptions: manufacturerOptions,
                access: "ALL",
                label: "Prevent Reset",
            }),
            mExt.actionEnumLookup({
                actionLookup: multiStateActions,
                cluster: "genMultistateInput",
                attribute: "presentValue",
                commands: ["attributeReport", "readResponse"],
            }),
        ],
    },
];
