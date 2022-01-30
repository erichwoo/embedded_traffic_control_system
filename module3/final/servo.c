/*
 * servo.c
 */

#include "servo.h"

/* Defines */
#define CTR_MAX		0xFFFFFFFF	/* 32 bit counter, 8xF */
#define CLOCK_FREQ	50000000	/* 50 MHz produced by FCLK_CLK0 */
#define WAVE_PERIOD 20			/* in milliseconds */

static XTmrCtr tmrCtr;

/* Helper function for resetting */
static u32 calcResetValue(double period){
	return 2 + CTR_MAX - (u32)(period*1e6/XTC_HZ_TO_NS(CLOCK_FREQ));
}

/*
 * Initialize the servo, setting the duty cycle to 7.5%
 */
void servo_init(void){
	// handle initialization for timer
	XTmrCtr_Initialize(&tmrCtr, XPAR_AXI_TIMER_0_DEVICE_ID);
	XTmrCtr_SetOptions(&tmrCtr, XTC_TIMER_0, XTmrCtr_GetOptions(&tmrCtr, XTC_TIMER_0) | XTC_PWM_ENABLE_OPTION | XTC_EXT_COMPARE_OPTION);
	XTmrCtr_SetOptions(&tmrCtr, XTC_TIMER_1, XTmrCtr_GetOptions(&tmrCtr, XTC_TIMER_1) | XTC_PWM_ENABLE_OPTION | XTC_EXT_COMPARE_OPTION);

	// timer 0 controls the whole waveform period
	XTmrCtr_SetResetValue(&tmrCtr, XTC_TIMER_0, calcResetValue(WAVE_PERIOD));
	// timer 1 controls the duty cycle
	servo_set(SERVO_MID);

	// start the timer
	XTmrCtr_Start(&tmrCtr, XTC_TIMER_0);
	XTmrCtr_Start(&tmrCtr, XTC_TIMER_1);
}

/*
 * Set the duty cycle of the servo
 */
void servo_set(double dutycycle){
	// dutycycle in percent
	if (dutycycle >= SERVO_MIN && dutycycle <= SERVO_MAX)
		XTmrCtr_SetResetValue(&tmrCtr, XTC_TIMER_1, calcResetValue(dutycycle*WAVE_PERIOD/100));
}
