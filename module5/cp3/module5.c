/*
 * module5.c -- builds upon module 4 and adds WiFi ADC functionality
 *
 *	module 3 useful values for interrupts??
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
#include "ttc.h"		/* triple timer counter on ps */
#include "servo.h"		/* servo module controlled by axi timer */
#include "adc.h"		/* adc module */
#include "uart.h"		/* uart module */

// led 4 status variable, flipped in the ttc_callback
static bool led4IsOn;
const static u32 led4Freq = 1;
// static double duty;
static bool done = false;

void ttc_callback(void) {
	led_set(4, !led4IsOn);
	led4IsOn = !led4IsOn;
}

void sw_callback(u32 sw) {
	led_toggle(sw);
}

void btn_callback(u32 btn) {
	led_toggle(btn);
//	if(btn == 0) printf("[Temp=%.2fc]\n>", adc_get_temp());
//	if(btn == 1) printf("[VccInt=%.2fv]\n>", adc_get_vccint());
//	if(btn == 2) printf("[Pot=%.2fv]\n>", adc_get_pot());
//	if(btn == 3){
//		duty = SERVO_MIN + (float)((u16)(adc_get_pot()*100 + .5)*(SERVO_MAX-SERVO_MIN))/100;
//		servo_set(duty);
//		printf("[Pot=%.2fv]\n>", adc_get_pot());
//		printf("duty cycle set to %.2lf%%\n>", duty);
//	}
	// fflush(stdout);
	done = btn == 3;
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
	while(!done){
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
