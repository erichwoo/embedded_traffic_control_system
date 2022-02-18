/*
 * module5.c -- builds upon module 4 and adds WiFi ADC functionality
 *
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

#define SERVER_ID_1 27								// module 5 server IDs (based on roster)
#define PING_BYTE_COUNT sizeof(ping_t)
#define UPDATE_BYTE_COUNT sizeof(update_response_t)

#define INT_MAX_DIGITS 11
#define UPDATE_VALUES_LEN 30

#define PING_MSG_LEN 28 + 2*INT_MAX_DIGITS 													// 28-char fixed response + 2 integers
#define UPDATE_MSG_LEN 49 + (UPDATE_VALUES_LEN-1)*2 + (UPDATE_VALUES_LEN+3)*INT_MAX_DIGITS  // 49-char fixed response + (arrayLen-1) 2-char delimiters + (arrayLen + 3) integers
#define ERROR_MSG_LEN 59

/* Program Variables/Defines */
static bool led4IsOn;			// state variable for led4's current on/off state, flipped in ttc_callback()
const static u32 led4Freq = 1;	// frequency of each led 4 on/off (a value of 1 is .5 Hz)
// static double duty;			// for the servo position (duty cycle as a percent)
u8 mode = CONFIGURE;			// the mode of our UI, accessible by uart.h as well

// to server
const static ping_t ping = {PING, SERVER_ID_1};
static update_request_t update = {UPDATE, SERVER_ID_1, 0};
// from server
static ping_t pong;
static update_response_t serverUpdate;

static u8 pingCount = 0;
static u8 updateCount = 0;

static int updateSign = 1;		// 1 = positive, -1 = negative
static bool updateValid = true;
static bool leadingZero = false;

static void (*wifi_callbacks[NUM_WIFI_CALLBACKS])(u8 buffer);

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

	switch (mode) {
	case CONFIGURE:
		printf("[CONFIGURE]\n");
		break;
	case PING:
		printf("[PING]\n");
		//ping_t ping = {PING, serverID1};
		uart_send(WIFI_DEV, (void*) &ping, sizeof(ping_t));
		break;
	case UPDATE:
		printf("[UPDATE]\n");
		break;
	case DONE:
		printf("[DONE]\n");
	default:
		break;
	}
}

void configure_request_callback(u8 buffer) {
	uart_send(WIFI_DEV, (void*)&buffer, TRIG_LEVEL);
}

void configure_response_callback(u8 buffer) {
	uart_send(TTY, (void*)&buffer, TRIG_LEVEL);
}

void ping_callback(u8 buffer) {}

void pong_callback(u8 buffer) {
	// receive each byte and store it at the next available byte
	u8* pingRecv = (u8*) &pong + pingCount;
	*pingRecv = buffer;
	pingCount++;

	// if ping_t is fully stored, print the ping message
	if (pingCount == PING_BYTE_COUNT) {
		char pingMsg[PING_MSG_LEN];
		int msg_len = sprintf(pingMsg, "server sent: [type=%d, id=%d]\n\r", pong.type, pong.id);
		if(msg_len >= 0)
			uart_send(TTY, (void*)pingMsg, (u32)msg_len);
		pingCount = 0;
	}
}

void update_request_callback(u8 buffer) {
	// carriage return indicates end of update value input
	if (buffer == (u8)'\r'){
		// send valid numbers off
		if (updateValid)
			uart_send(WIFI_DEV, (void*)&update, sizeof(update_request_t));
		else {
			char errorMsg[ERROR_MSG_LEN];
			int msg_len = sprintf(errorMsg, "Invalid input: must enter a valid 32-bit signed integer!\n\r");
			if(msg_len >= 0)
				uart_send(TTY, (void*)errorMsg, (u32)msg_len);
		}
		// reset updateVal
		update.value = 0;
		updateValid = true;
		leadingZero = false;
		updateSign = 1;

		// newline
		u8 newLine = (u8)'\n';
		uart_send(TTY, (void*)&newLine, TRIG_LEVEL);
	}
	// set leading zero
	else if (update.value == 0 && buffer == (u8)'0')
		leadingZero = true;
	else if (updateValid) {
		int digit = updateSign * (int)(buffer - (u8)'0');

		// if '-' at start of input typing
		if(buffer == (u8) '-' && update.value == 0 && !leadingZero)
			updateSign = -1;

		// not a digit, or leading zero, or overflow
		else if ( buffer < (u8)'0' || buffer > (u8)'9' || leadingZero     ||
				  (updateSign > 0 && update.value > (INT_MAX - digit)/10) ||
				  (updateSign < 0 && update.value < (INT_MIN - digit)/10)    )
			updateValid = false;

		// general case
		else {
			update.value *= 10;
			update.value += digit;
		}
	}

	// echo back to TTY
	uart_send(TTY, (void*)&buffer, TRIG_LEVEL);
}

void update_response_callback(u8 buffer) {
	// receive each byte and store it at the next available byte
	u8* updateRecv = (u8*) &serverUpdate + updateCount;
	*updateRecv = buffer;
	updateCount++;

	// if server_response_t is fully stored, print the update message
	if (updateCount == UPDATE_BYTE_COUNT) {
		char updateMsg[UPDATE_MSG_LEN];

		int msg_len = sprintf(updateMsg, "server sent: [type=%d, id=%d, average=%d, values={", serverUpdate.type, serverUpdate.id, serverUpdate.average);
		//size_t arraySize = sizeof(serverUpdate.values)/sizeof(serverUpdate.values[0]);
		for (int i = 0; i < UPDATE_VALUES_LEN; i++) {
			msg_len += sprintf(updateMsg + msg_len, "%d", serverUpdate.values[i]);
			if (i < UPDATE_VALUES_LEN - 1)
				msg_len += sprintf(updateMsg + msg_len, ", ");
		}
		msg_len += sprintf(updateMsg + msg_len, "}]\n\r");

		if(msg_len >= 0)
			uart_send(TTY, (void*)updateMsg, (u32)msg_len);
		updateCount = 0;
	}
}

int main(){
	init_platform();

	// uart and gic
	gic_init();

	wifi_callbacks[C_TO] = configure_request_callback;
	wifi_callbacks[C_FRO] = configure_response_callback;
	wifi_callbacks[P_TO] = ping_callback;
	wifi_callbacks[P_FRO] = pong_callback;
	wifi_callbacks[U_TO] = update_request_callback;
	wifi_callbacks[U_FRO] = update_response_callback;
	uart_init(wifi_callbacks);

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
