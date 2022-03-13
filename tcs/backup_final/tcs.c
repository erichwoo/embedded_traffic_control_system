/*
 * tcs.c -- builds upon module 5 and adds FSM
 *
 */

/*************** C Standard Libraries **************/
#include <stdio.h>		/* getchar,printf */
#include <stdlib.h>		/* string to decimal, more helper functions */
#include <stdbool.h>	/* type bool */
#include <unistd.h>		/* sleep */
#include <string.h>		/* string manipulation */

/***************** Xilinx Libraries ****************/
#include "platform.h"		/* ZYBO board interface */
#include "xil_types.h"		/* u32, s32 etc */
#include "xparameters.h"	/* constants used by hardware */
#include "xgpio.h"			/* axi gpio interface */

/*************** User-Defined Modules **************/
#include "led.h"		/* LED Module */
#include "io.h"			/* io module, buttons & switches */
#include "gic.h"		/* general interrupt controller interface, provided by  */
#include "ttc.h"		/* triple timer counter on ps */
#include "servo.h"		/* servo module controlled by axi timer */
#include "adc.h"		/* adc module */
//#include "wifi.h"		/* wifi module */
#include "fsm.h"

/********************* DEFINES **********************/
#define TTC_FREQ 10

//static double gate_pos = OPEN;

/***************************** MAIN *************************/
void init(void) {
	// platform initialization
	init_platform();

	// gic initialization
	gic_init();

	//uart initialization
	uart_init(&update_response_callback);

	// btn & sw initialization
	io_btn_init(&btn_callback);
	io_sw_init(&sw_callback);

	// led initialization
	led_init();

	// led6 initialization
	led6_init();

	// ttc initialization
	ttc_init(TTC_FREQ, &ttc_callback);

	// servo initialization
	servo_init();
	servo_set(OPEN);

	// XADC initialization
	adc_init();
}

void destroy(void) {
	// close gic interrupts
	//uart_close();
	io_sw_close();
	io_btn_close();
	ttc_close();

	// close gic
	gic_close();

	// turn off all leds at close
	led_set(ALL, LED_OFF);

	printf("All Peripherals closed. Goodbye!\n");

	// cleanup the hardware platform
	cleanup_platform();
	sleep(1);
}

int main(){
	// initialize
	init();

	// main
	printf("[hello]\n");
	init_state();
	while(get_state() != DONE){
		sleep(1);
	}
	printf("\n---- main while loop done ----\n");

	// close
	destroy();
	return 0;
}
