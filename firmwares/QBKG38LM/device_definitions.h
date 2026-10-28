#ifndef DEVICE_DEFINITIONS_H
#define DEVICE_DEFINITIONS_H

/* Button and LED pins measured on A20-WSKE1W-NNS-CN (chip pin -> PIO): button 16 -> PIO13, blue LED 19 -> PIO16,
 * red LED 25 -> PIO21, second blue LED 3 -> PIO0, second red LED 6 -> PIO3, relay reset coil 21 -> PIO17, relay set coil 22 -> PIO18, relay rail supply 24 -> PIO20.
 * Single rocker: both button pads are one net, one latching relay (L1). */

/* Button defintions */
#define BTN_CENTER_DIO (13)  // rocker button DIO (chip pin 16)
#define BTN_CENTER_MASK (1 << BTN_CENTER_DIO)
#define BTN_INTERRUPT_MASK BTN_CENTER_MASK
#define BTN_RESET_MASK BTN_INTERRUPT_MASK
/* Button defintions - end */

/* Led definitions (active low) */
/* Two LED pairs (one on each side of the rocker), lit together */
#define LED_BLUE_DIO (16)   // chip pin 19
#define LED_RED_DIO (21)    // chip pin 25
#define LED_BLUE2_DIO (0)   // chip pin 3
#define LED_RED2_DIO (3)    // chip pin 6
#define LED_CENTER_MASK ((1UL << LED_BLUE_DIO) | (1UL << LED_BLUE2_DIO))
#define RESET_LED_MASK ((1UL << LED_RED_DIO) | (1UL << LED_RED2_DIO))
/* Led definitions - end */

/* Latching relay (HF182F-L/3-HL2TF, two coils, common "+" on the 1000 uF supply rail) definitions, measured on this
 * board: each coil "-" goes to its own low-side transistor, enabled by a high level. Datasheet: 11.25 Ohm per 3 V coil,
 * pulse >= 50 ms, never energize both coils at once. */
#define RELAY_SET_DIO (18)    // chip pin 22, closes L-L1
#define RELAY_RESET_DIO (17)  // chip pin 21, opens L-L1
#define RELAY_PULSE_MSEC (50)
/* The coil "+" rail is charged through a two-transistor high-side switch: chip pin 24 -> PIO20, active high (low-side
 * NPN/N-MOS driving the high-side transistor). Without it the rail stays at 0 V on mains. */
#define RELAY_SUPPLY_DIO (20)
#define RELAY_SUPPLY_DELAY_MSEC (500)  // after boot, before the rail supply is switched on
#define RELAY_CHARGE_MSEC (1000)       // rail charging time before the first coil pulse
/* Latching relay definitions - end */

/* Wall switch without neutral: no battery, the radio stays on and the device never sleeps */
#define DEVICE_MAINS_POWERED

#define LEDS_AMOUNT 1
#define BUTTONS_AMOUNT 1

#endif /* DEVICE_DEFINITIONS_H */
