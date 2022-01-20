/*
 * main.c -- A program to print a dot each time button 0 is pressed.
 *
 *  Some useful values:
 *  -- XPAR_AXI_GPIO_1_DEVICE_ID -- xparameters.h
 *  -- XPAR_FABRIC_GPIO_1_VEC_ID -- xparameters.h
 *  -- XGPIO_IR_CH1_MASK         -- xgpio_l.h (included by xgpio.h)
 */
#include <stdio.h>		/* getchar,printf */
#include <stdlib.h>		/* strtod */
#include <stdbool.h>	/* type bool */
#include <unistd.h>		/* sleep */
#include <string.h>

#include "platform.h"		/* ZYBO board interface */
#include "xil_types.h"		/* u32, s32 etc */
#include "xparameters.h"	/* constants used by hardware */

#include "led.h"		/* LED Module */
#include "io.h"			/* io module */
#include "gic.h"		/* interrupt controller interface */
#include "xgpio.h"		/* axi gpio interface */
#include "ttc.h"

void btn_callback(u32 btn) {
	led_toggle(btn);
}

void sw_callback(u32 sw) {
	led_toggle(sw);
}

void ttc_callback(void) {
	led_set(4, LED_ON);
	printf("blink\n");
	led_set(4, LED_OFF);
}

int main() {
	init_platform();

	/* initialize the gic (c.f. gic.h) */
	gic_init();

	// initialize the buttons using io module
	io_btn_init(&btn_callback);
	io_sw_init(&sw_callback);

	// initialize LED module
	led_init();
	led_set(4, LED_ON);

	ttc_init(1, &ttc_callback);
	ttc_start();
	printf("gic, io, led initialized...\n");
	printf("[hello]\n"); /* so we are know its alive */

	// create input buffer of static 64 len
	setvbuf(stdin,NULL,_IONBF,0);
	const int len = 64;
	char buffer[len];

	// while input line is not "quit"
	while (strcmp(buffer, "q") != 0) {
		// start new input line
		printf(">");

		// initialize ptr to end of string/input
		int i = 0;
		buffer[i] = '\0';
		char c;
		// while character isn't newline, keep reading
		while ( (c = getchar()) != '\r') {
			printf("%c", c);  // echo back char
			fflush(stdout);

			// write char to end of buffer
			if(i >= len - 1){  // safety check, wrap back to beginning of buffer
				i = 0;
			}
			buffer[i++] = c;
			buffer[i] = '\0';
		}
		printf("\n");

		// if single char inputted, check for 0-3, r,g,b,y
		if (i == 1) {
			long val = strtol(buffer, NULL, 10); // grab value of buffer in case of 0-3
			switch(buffer[0]) { // check first char in buffer
				case '0':
				case '1':
				case '2':
				case '3':
					printf("[%s]\n", buffer);
					printf("Toggling led %lu...\n", val);
					led_toggle((u32)val);
				default:
					break;
			}
		}
	}
	printf("\nUI [done]\n");

	/* close the gic (c.f. gic.h)*/
	io_sw_close();
	io_btn_close();
	ttc_close();
	gic_close();
	printf("gic closed\n");

	// turn off all leds at close
	led_set(ALL, LED_OFF);

	cleanup_platform();					/* cleanup the hardware platform */
	printf("---- program exits here ----\n");
	return 0;
}
