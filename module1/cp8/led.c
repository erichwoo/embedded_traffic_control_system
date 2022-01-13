/*
 * Erich Woo & Wendell Wu
 * ENGS 62 Module 1
 * 9 Jan 2022
 */

#include "led.h"

static XGpio port;

/*
 * Initialize the led module
 */
void led_init(void) {
	XGpio_Initialize(&port, XPAR_AXI_GPIO_0_DEVICE_ID);	/* initialize device AXI_GPIO_0 */
	XGpio_SetDataDirection(&port, CHANNEL1, OUTPUT);	    /* set tristate buffer to output */
}

/*
 * Set <led> to one of {LED_ON,LED_OFF,...}
 *
 * <led> is either ALL or a number >= 0
 * Does nothing if <led> is invalid
 */
void led_set(u32 led, bool tostate) {
	u32 prev = XGpio_DiscreteRead(&port, CHANNEL1);
	u32 mask = (led == ALL) ? 0xF : (1 << led);

	if (tostate) mask |= prev;
	else mask = ~mask & prev;

	XGpio_DiscreteWrite(&port, CHANNEL1, mask);
}

/*
 * Get the status of <led>
 *
 * <led> is a number >= 0
 * returns {LED_ON,LED_OFF,...}; LED_OFF if <led> is invalid
 */
bool led_get(u32 led) {
	u32 bits = XGpio_DiscreteRead(&port, CHANNEL1); // current state of port
	u32 mask = (0x1 << led); // used to read the specific bit
	return ((mask & bits) > 0) ? LED_ON : LED_OFF; // if was on, then value would be greater than 0
}

/*
 * Toggle <led>
 *
 * <led> is a value >= 0
 * Does nothing if <led> is invalid
 */
void led_toggle(u32 led) {
	XGpio_DiscreteWrite(&port, CHANNEL1, (1 << led) ^ XGpio_DiscreteRead(&port, CHANNEL1));
}

