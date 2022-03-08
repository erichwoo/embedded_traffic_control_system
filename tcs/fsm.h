/*
 * Header file for fsm.c
 */

#pragma once

#include "unistd.h"
#include "led.h"		/* LED Module */
#include "io.h"			/* io module, buttons & switches */
#include "gic.h"		/* general interrupt controller interface, provided by  */
#include "ttc.h"		/* triple timer counter on ps */
#include "servo.h"		/* servo module controlled by axi timer */
#include "adc.h"		/* adc module */
//#include "wifi.h"		/* wifi module */

#define DONE		-1

// default states
#define PEDESTRIAN	0
#define Y2G			1
#define Y2R			2
#define V_MIN		3
#define V_OK		4
#define V_MIN_PED	5

// train states
#define TRAIN		6
#define Y_TRAIN		7
#define PED_TRAIN	8
#define Y_TRAIN_CLR	9

// maintenence states
#define MAINTENANCE	10
#define M_TRAIN		11
#define M_CLR		12

// interrupt timing in secs
#define PED_TIME 	10
#define LIGHT_TIME	3
#define V_MIN_TIME  10

#define PED_LIGHT	4
#define TM_LIGHT	6

// transitions
#define M_SW_HI		0
#define M_SW_LO		1
#define T_SW_HI		2
#define T_SW_LO		3
#define P_BTN		4
#define T_INT		5

void ttc_callback(void);
void btn_callback(u32 btn);
void sw_callback(u32 sw);
int get_state(void);
void init_state(void);

//static void load_timer(int interrupt);

//static void change_state(int transition);
//static void generate_outputs(void);
