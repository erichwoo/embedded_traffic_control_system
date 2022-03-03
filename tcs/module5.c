/*
 * module5.c -- builds upon module 4 and adds WiFi ADC functionality
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
#include "wifi.h"		/* wifi module */

/******************* DEFINES ***********************/
#define WIFI_DEV 0
#define TTY 	 1

#define SERVER_ID_1 27								// module 5 server IDs (based on roster)
#define SERVER_ID_2 28

#define PING_BYTE_COUNT   sizeof(ping_t)
#define UPDATE_BYTE_COUNT sizeof(update_response_t)

#define INT_MAX_DIGITS 	  11
#define UPDATE_VALUES_LEN 30

#define PONG_MSG_LEN 	28 + 2*INT_MAX_DIGITS 												 // 28-char fixed response + 2 integers
#define UPDATE_MSG_LEN 	49 + (UPDATE_VALUES_LEN-1)*2 + (UPDATE_VALUES_LEN+3)*INT_MAX_DIGITS  // 49-char fixed response + (arrayLen-1) 2-char delimiters + (3 + arrayLen) integers
#define ERROR_MSG_LEN 	59																	 // 59-char fixed response

/********************* GLOBALS ************************/
u8 mode = CONFIGURE;			// the mode of our UI, accessible by wifi.h as well

/**************** STATIC VARIABLES ********************/
// leds
static bool led4IsOn;			// state variable for led4's current on/off state, flipped in ttc_callback()
static u32 led4Freq = 1;		// frequency of each led 4 on/off (a value of 1 is .5 Hz)

// servo
static double duty;				// for the servo position (duty cycle as a percent)

// to server
const static ping_t ping = {PING, SERVER_ID_1};
static update_request_t update = {UPDATE, SERVER_ID_1, 0};
static int updateSign = 1;		// 1 = positive, -1 = negative
static bool updateValid = true;
static bool leadingZero = false;

// from server
static ping_t pong;
static update_response_t updateResponse;
static u8 pongCount = 0;
static u8 updateCount = 0;

/***************** STATIC FUNCTIONS **********************/
static void (*wifi_callbacks[NUM_WIFI_CALLBACKS])(u8 buffer);

/************** USER-DEFINED CALLBACKS ********************/
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
		uart_send(WIFI_DEV, (void*) &ping, sizeof(ping_t));
		break;
	case UPDATE:
		printf("[UPDATE]\n");
		update.value = (int)adc_get_pot_percent();
		uart_send(WIFI_DEV, (void*) &update, sizeof(update_request_t));
		update.value = 0;
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
	u8* pongRecv = (u8*) &pong + pongCount;
	*pongRecv = buffer;
	pongCount++;

	// if ping_t is fully stored, print the ping message
	if (pongCount == PING_BYTE_COUNT) {
		char pongMsg[PONG_MSG_LEN];
		int msg_len = sprintf(pongMsg, "server sent: [type=%d, id=%d]\n\r", pong.type, pong.id);
		if(msg_len >= 0) uart_send(TTY, (void*)pongMsg, (u32)msg_len);
		pongCount = 0;
	}
}

void update_request_callback(u8 buffer) {
	// echo back to TTY
	uart_send(TTY, (void*)&buffer, TRIG_LEVEL);

	// carriage return indicates end of update value input
	if (buffer == (u8)'\r'){
		// send valid numbers off
		if (updateValid) uart_send(WIFI_DEV, (void*)&update, sizeof(update_request_t));
		// otherwise print error message
		else {
			char errorMsg[ERROR_MSG_LEN];
			int msg_len = sprintf(errorMsg, "Invalid input: must enter a valid 32-bit signed integer!\n\r");
			if(msg_len >= 0) uart_send(TTY, (void*)errorMsg, (u32)msg_len);
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
	// if current integer input is still valid, keep appending to it
	else if (updateValid) {
		int digit = updateSign * (int)(buffer - (u8)'0'); // next digit to add/subtract

		// if '-' at start of input typing, set to subtract each digit
		if(buffer == (u8) '-' && update.value == 0 && !leadingZero)
			updateSign = -1;

		// not a digit, or leading zero, or overflows
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
}

void update_response_callback(u8 buffer) {
	// receive each byte and store it at the next available byte
	u8* updateRecv = (u8*) &updateResponse + updateCount;
	*updateRecv = buffer;
	updateCount++;

	// if server_response_t is fully stored, print the update message
	if (updateCount == UPDATE_BYTE_COUNT) {
		// update servo
		duty = servo_set_percent(updateResponse.values[SERVER_ID_1]);

		// print update message
		char updateMsg[UPDATE_MSG_LEN];

		int msg_len = sprintf(updateMsg, "server sent: [type=%d, id=%d, average=%d, values={", updateResponse.type, updateResponse.id, updateResponse.average);
		for (int i = 0; i < UPDATE_VALUES_LEN; i++) {
			msg_len += sprintf(updateMsg + msg_len, "%d", updateResponse.values[i]);
			if (i < UPDATE_VALUES_LEN - 1) msg_len += sprintf(updateMsg + msg_len, ", ");
		}
		msg_len += sprintf(updateMsg + msg_len, "}]\n\r");

		if(msg_len >= 0) uart_send(TTY, (void*)updateMsg, (u32)msg_len);
		updateCount = 0;
	}
}

/***************************** MAIN *************************/
void init(void) {
	// platform initialization
	init_platform();

	// gic initialization
	gic_init();

	// uart initialization
	wifi_callbacks[C_TO] = configure_request_callback;
	wifi_callbacks[C_FRO] = configure_response_callback;
	wifi_callbacks[P_TO] = ping_callback;
	wifi_callbacks[P_FRO] = pong_callback;
	wifi_callbacks[U_TO] = update_request_callback;
	wifi_callbacks[U_FRO] = update_response_callback;
	uart_init(wifi_callbacks);

	// btn & sw initialization
	io_btn_init(&btn_callback);
	io_sw_init(&sw_callback);

	// led initialization
	led_init();
	led_set(4, LED_ON);
	led4IsOn = LED_ON;

	led6_init();
	led6_set(R);

	// ttc initialization
	ttc_init(led4Freq, &ttc_callback);
	ttc_start();

	// servo initialization
	servo_init();
	duty = SERVO_MID;

	// XADC initialization
	adc_init();
}

void destroy(void) {
	// close gic interrupts
	uart_close();
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

/*int main(){
	// initialize
	init();

	// main
	printf("[hello]\n");
	while(mode != DONE){
		sleep(1);
	}
	printf("\n---- main while loop done ----\n");

	// close
	destroy();
	return 0;
}*/
