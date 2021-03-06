/*
 * led.h -- led module interface
 *
 */
#pragma once

#include <stdio.h>
#include <stdbool.h>
#include <xgpio.h>		  	/* axi gpio */
#include <xgpiops.h>		/* processor gpio */
#include "xparameters.h"  	/* constants used by the hardware */
#include "xil_types.h"		/* types used by xilinx */

/* led states */
#define LED_ON true
#define LED_OFF false

#define ALL 0xFFFFFFFF		/* A value designating ALL leds */

#define OUTPUT 0x0							/* setting GPIO direction to output */
#define CHANNEL1 1							/* channel 1 of the GPIO port */

#define OUTPUT_PS 1			// Processing System Output direction
#define MIO7 7				// Pin number (LED 4, MIO 7 connected directly to PS)
#define OP_EN 1				// Processing System Output Enable

// r,g,b,y,w colors for led6
#define R (1 << 2)
#define G (1 << 1)
#define B (1 << 0)
#define Y (R+G)
#define W (R+G+B)

/*
 * Initialize the led module
 */
void led_init(void);

/*
 * Set <led> to one of {LED_ON,LED_OFF,...}
 *
 * <led> is either ALL or a number >= 0
 * Does nothing if <led> is invalid
 */
void led_set(u32 led, bool tostate);

/*
 * Get the status of <led>
 *
 * <led> is a number >= 0
 * returns {LED_ON,LED_OFF,...}; LED_OFF if <led> is invalid
 */
bool led_get(u32 led);

/*
 * Toggle <led>
 *
 * <led> is a value >= 0
 * Does nothing if <led> is invalid
 */
void led_toggle(u32 led);

/**
 * Set the color of led6 (assumed always on)
 *
 * colors:
 * - r = 0b100 aka 4
 * - g = 0b010 aka 2
 * - b = 0b001 aka 1
 * - y = 0b110 aka 6
 */
void led6_set(u32 color);

/**
 * Special function to initialize led6
 */
void led6_init(void);
