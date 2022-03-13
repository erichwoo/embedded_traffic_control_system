/*
 * Header file for fsm.c
 */

#pragma once

#include <unistd.h> 	/* sleep */
#include <stdbool.h>    /* toggle blue light functionality */
#include "led.h"		/* LED Module */
#include "io.h"			/* io module, buttons & switches */
#include "gic.h"		/* general interrupt controller interface, provided by  */
#include "ttc.h"		/* triple timer counter on ps */
#include "servo.h"		/* servo module controlled by axi timer */
#include "adc.h"		/* adc module */
#include "wifi.h"		/* wifi module */

#define DONE 		-1

// default states
#define PEDESTRIAN	0
#define Y2G			1
#define V_MIN		2
#define V_OK		3
#define V_MIN_PED	4
#define Y2R			5

// train states
#define Y_TRAIN		6
#define TRAIN		7
#define PED_TRAIN	8

// maintenance states
#define MAINTENANCE	9
#define M_TRAIN		10
#define M_CLR		11

// interrupt timing in secs
#define PED_TIME 	10
#define LIGHT_TIME	3
#define V_MIN_TIME  10
#define BLUE_TIME	1

#define PED_LIGHT	4

// transitions
#define M_SW_HI		0
#define M_SW_LO		1
#define T_SW_HI		2
#define T_SW_LO		3
#define P_BTN		4
#define T_INT		5
#define DEFAULT		6

// I/O defines

// gate position
#define OPEN 	SERVO_MAX
#define CLOSED 	SERVO_MIN

// Wifi Module
#define REQUEST_ID		0
#define DUMMY			0
#define SERVER_START	-1
/******************** FUNCTION DECLARATIONS **************************/

// Peripheral Callbacks
void ttc_callback(void);
void btn_callback(u32 btn);
void sw_callback(u32 sw);
void update_response_callback(u8 buffer);

// exposed FSM functions
int get_state(void);
void init_state(void);
