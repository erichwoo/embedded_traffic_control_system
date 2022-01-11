/*
 * Erich Woo & Wendell Wu
 * ENGS 62 Module 1
 * 9 Jan 2022
 */

#include "led.h"

static XGpio port;
static XGpio port6; // just for led 6
static XGpioPs portPs;

void led_init(void) {
	// AXI-GPIO device0: led0-3
	XGpio_Initialize(&port, XPAR_AXI_GPIO_0_DEVICE_ID);	/* initialize device AXI_GPIO_0 */
	XGpio_SetDataDirection(&port, CHANNEL1, OUTPUT);	    /* set tristate buffer to output */

	// PS7-GPIO device0: led4
	XGpioPs_CfgInitialize(&portPs, XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID), XPAR_PS7_GPIO_0_BASEADDR); // XPAR_PS7_GPIO_0_BASEADDR
	XGpioPs_SetDirectionPin(&portPs, MIO7, OUTPUT_PS);
	XGpioPs_SetOutputEnablePin(&portPs, MIO7, OP_EN);

	// AXI-GPIO device1: led6
	XGpio_Initialize(&port6, XPAR_AXI_GPIO_1_DEVICE_ID);	/* initialize device AXI_GPIO_0 */
	XGpio_SetDataDirection(&port6, CHANNEL1, OUTPUT);	    /* set tristate buffer to output */
}

void led_set(u32 led, bool tostate) {
	// handle led4 separately
	if (led == 4 || led == ALL) {
		XGpioPs_WritePin(&portPs, MIO7, tostate ? 0x1 : 0x0);
		if (led == 4) return;
	}

	// set 4 leds
	u32 prev = XGpio_DiscreteRead(&port, CHANNEL1);
	u32 mask = (led == ALL) ? 0xF : (0x1 << led); // either all leds, bitshift 1 to the correct led

	if (tostate) mask |= prev; // OR with the previous bit states
	else mask = ~mask & prev;  // 0-out mask, then AND in order to 0 out the correct leds

	XGpio_DiscreteWrite(&port, CHANNEL1, mask);

	// turn off port6
	if (!tostate && led == ALL)
		XGpio_DiscreteWrite(&port6, CHANNEL1, XGpio_DiscreteRead(&port6, CHANNEL1) & ~W); // rmw

}

bool led_get(u32 led) {
	if (led == 4) return XGpioPs_ReadPin(&portPs, MIO7) == 0x1;

	u32 bits = XGpio_DiscreteRead(&port, CHANNEL1); // current state of port
	u32 mask = (0x1 << led); // used to read the specific bit
	return ((mask & bits) > 0) ? LED_ON : LED_OFF; // if was on, then value would be greater than 0
}

void led_toggle(u32 led) {
	//if (led == 4) XGpioPs_WritePin(&portPs, MIO7, 1 ^ XGpioPs_ReadPin(&portPs, MIO7));
	//else XGpio_DiscreteWrite(&port, CHANNEL1, (0x1 << led) ^ XGpio_DiscreteRead(&port, CHANNEL1));

	XGpio_DiscreteWrite(&port, CHANNEL1, (0x1 << led) ^ XGpio_DiscreteRead(&port, CHANNEL1));
}

void led6_set(u32 color){
	u32 mask = XGpio_DiscreteRead(&port6, CHANNEL1) & ~(W);
	XGpio_DiscreteWrite(&port6, CHANNEL1, color | mask); // rmw
}
