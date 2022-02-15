/*
 * module5.c -- builds upon module 4 and adds WiFi ADC functionality
 *
 *	module 3 useful values for interrupts??
 *  Some useful values:
 *  -- XPAR_AXI_GPIO_1_DEVICE_ID -- xparameters.h
 *  -- XPAR_FABRIC_GPIO_1_VEC_ID -- xparameters.h
 *  -- XGPIO_IR_CH1_MASK         -- xgpio_l.h (included by xgpio.h)
 */

/* C Standard Libraries */
#include <stdio.h>		/* getchar,printf */
#include <stdlib.h>		/* string to decimal, more helper functions */
#include <stdbool.h>	/* type bool */
#include <unistd.h>		/* sleep */
#include <string.h>		/* string manipulation */

/* Xilinx Libraries */
#include "platform.h"		/* ZYBO board interface */
#include "xil_types.h"		/* u32, s32 etc */
#include "xparameters.h"	/* constants used by hardware */
#include "xgpio.h"			/* axi gpio interface */

/* User-Defined Modules */
#include "led.h"		/* LED Module */
#include "io.h"			/* io module, buttons & switches */
#include "gic.h"		/* general interrupt controller interface, provided by  */
#include "ttc.h"		/* triple timer counter on ps */
#include "servo.h"		/* servo module controlled by axi timer */
#include "adc.h"		/* adc module */
#include "wifi.h"		/* wifi module */

/* User Types */
#define WIFI_DEV 0
#define TTY 1

/* Program Variables/Defines */
static bool led4IsOn;			// state variable for led4's current on/off state, flipped in ttc_callback()
const static u32 led4Freq = 1;	// frequency of each led 4 on/off (a value of 1 is .5 Hz)
// static double duty;			// for the servo position (duty cycle as a percent)
u8 mode = CONFIGURE;			// the mode of our UI, accessible by uart.h as well
// module 5 server IDs (based on roster)
const static u32 serverID1 = 27;

void ttc_callback(void) {
	led_set(4, !led4IsOn);
	led4IsOn = !led4IsOn;
}

void sw_callback(u32 sw) {
	led_toggle(sw);
}

void btn_callback(u32 btn) {
	led_set(ALL, LED_OFF);
	led_set(btn, LED_ON);
	mode = (u8) btn;

	if (mode == CONFIGURE)
		printf("[CONFIGURE]\n");
	else if (mode == PING){
		printf("[PING]\n");
		ping_t ping = {PING, serverID1};
		uart_send(WIFI_DEV, (void*) &ping, sizeof(ping_t));
	}
	else if (mode == UPDATE)
		printf("[UPDATE]\n");
}

int main(){
	init_platform();

	// uart and gic
	gic_init();
	uart_init();

	// initialize the buttons using io module
	io_btn_init(&btn_callback);
	io_sw_init(&sw_callback);

	// initialize LED module
	led_init();
	led_set(4, LED_ON);
	led4IsOn = LED_ON;

	// ttc initialization
	ttc_init(led4Freq, &ttc_callback);
	ttc_start();

	printf("[hello]\n");	// so we know we are alive
	while(mode != DONE){
		sleep(1);
	}

	printf("\n---- main while loop done ----\n");

	// close the gic (c.f. gic.h)
	uart_close();
	io_sw_close();
	io_btn_close();
	ttc_close();
	gic_close();
	// turn off all leds at close
	led_set(ALL, LED_OFF);

	printf("All Peripherals closed. Goodbye!\n");

	cleanup_platform();		// cleanup the hardware platform

	sleep(1);
	return 0;
}

/*


int main() {
	servo_init();
	duty = SERVO_MID;
	printf("servo initialized and started...\n");

	adc_init();
	printf("XADC Module initialized and started...\n");

	printf("[hello]\n"); // so we know we are alive

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
				case 'a':
					if (duty >= SERVO_MAX) {
						printf("Error: Max Value %.2lf Reached\n", duty);
						break;
					}
					duty += 0.25;
					printf("duty cycle increased to %.2lf%%\n", duty);
					break;
				case 's':
					if (duty <= SERVO_MIN) {
						printf("Error: Min Value %.2lf Reached\n", duty);
						break;
					}
					duty -= 0.25;
					printf("duty cycle decreased to %.2lf%%\n", duty);
					break;
				case 'l':
					duty = SERVO_MIN;
					printf("duty cycle set to MIN of %.2lf%%\n", duty);
					break;
				case 'h':
					duty = SERVO_MAX;
					printf("duty cycle set to MAX of %.2lf%%\n", duty);
					break;
				case 'm':
					duty = SERVO_MID;
					printf("duty cycle set to MID of %.2lf%%\n", duty);
					break;
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
			servo_set(duty);
		}
	}
	printf("UI [done]\n");

	cleanup_platform();					// cleanup the hardware platform
	printf("---- program exits here ----\n");
	return 0;
}
*/
