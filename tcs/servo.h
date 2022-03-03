/*
 * servo.h
 */
#pragma once

#include <stdio.h>
#include "xtmrctr.h"
#include "xparameters.h"  	/* constants used by the hardware */
#include "xil_types.h"		/* types used by xilinx */

#define SERVO_MID	7.5	    /* in percent of WAVE_PERIOD */
#define SERVO_MAX   9.75    /* tested max percent for 45-degrees */
#define SERVO_MIN   5.25    /* tested min percent for 45-degrees */
/*
 * Initialize the servo, setting the duty cycle to 7.5%
 */
void servo_init(void);

/*
 * Set the dutycycle of the servo
 */
void servo_set(double dutycycle);

double servo_set_percent(u32 percent);
