/*
 * fsm.c -- Event-Driven Finite State Machine for Traffic Control System
 *
 */

#include "fsm.h"

#define WIFI_DEV 0
#define TTY 	 1

#define SERVER_ID 27								// module 5 server IDs (based on roster)

#define UPDATE_BYTE_COUNT sizeof(update_response_t)

#define FETCH_TRIGGER 3

/************************ STATIC FUNCTION DECLARATIONS ***********************/

static void blue_off(void);
static void reset_ttc(void);
static void restart_ttc(int trig);
static void change_state(int transition);
static void generate_outputs(void);

/****************************** STATIC VARIABLES *****************************/

static bool ttcIsOn = false;
static int counter = 0;             /* counter/divider for ttc, which operates at 10 Hz */
static int trigger;				    /* desired trigger (sec) to run new code */
static int state;					/* current FSM state */
static bool blueStatus = LED_OFF;   /* LED6 Blue-light status (On/Off) */

static int fetchCounter = 0;  /* fetch every 300ms > 250ms */

static update_request_t request = {UPDATE, SERVER_ID, SERVER_START};

static update_response_t response;
static u8 responseCount = 0;

static int remoteSwTrans; // server = 3
static bool init = true;

/**/
static void blue_off(void) {
	// turn off
	led6_set(OFF);
	blueStatus = LED_OFF;
}

static void restart_ttc(int trig) {
	trigger = trig;
	ttcIsOn = true;

	ttc_stop();
	ttc_reset();
	ttc_start();
}

static void reset_ttc(void) {
	counter = 0;
	ttcIsOn = false;
}

/****************************** PERIPHERAL CALLBACKS *******************************/

void ttc_callback(void) {
	// poll Wifi Module
	fetchCounter++;
	if (fetchCounter >= FETCH_TRIGGER) {
		uart_send(WIFI_DEV, (void*) &request, sizeof(update_request_t));
		fetchCounter = 0;
	}

	// FSM
	if (ttcIsOn) {
		counter++;

		// polling potentiometer if in MAITNENANCE STATES
		if (state == MAINTENANCE || state == M_TRAIN || state == M_CLR) {
			servo_set_percent(adc_get_pot_percent());
		}

		if (counter >= trigger*10) {
			if (state == MAINTENANCE || state == M_TRAIN || state == M_CLR) {
				//restart the counter
				counter = 0;

				// toggle blue
				blueStatus = !blueStatus;
				led6_set((blueStatus) ? B : OFF);
			}
			else {
				// reset and stop counter before changing state
				reset_ttc();
				// change state
				change_state(T_INT);
			}
		}
	}
}

void btn_callback(u32 btn) {
	if (btn == 3)
		change_state(DONE);
	else if (btn == 0 || btn == 1)
		change_state(P_BTN);
}

void sw_callback(u32 sw) {
	bool hi = ( (1 << sw) & io_sw_read() ) > 0; // checks whether bit position @ sw is set to hi/lo

	if (sw == 0 && hi) 	  	 change_state(M_SW_HI);
	else if (sw == 0 && !hi) change_state(M_SW_LO);
	else if (sw == 1 && hi)  change_state(T_SW_HI);
	else if (sw == 1 && !hi) change_state(T_SW_LO);
}

void update_response_callback(u8 buffer) {
	// receive each byte and store it at the next available byte
	u8* updateRecv = (u8*) &response + responseCount;
	*updateRecv = buffer;
	responseCount++;

	// if server_response_t is fully stored, print the update message
	if (responseCount == UPDATE_BYTE_COUNT) {
		int newTrans = response.values[SERVER_ID];

		// deal with server response
		if (init) {
			remoteSwTrans = newTrans;
			init = false;
		}
		else {
			if (newTrans >= M_SW_HI && newTrans <= T_SW_LO && newTrans != remoteSwTrans) {
				remoteSwTrans = newTrans;
				change_state(newTrans);
			}
			else remoteSwTrans = newTrans;
		}

		// reset update
		responseCount = 0;
	}
}

/************************************** FSM LOGIC ********************************/

void init_state(void) {
	// synchronize w/ server value, setting to default -1
	uart_send(WIFI_DEV, (void*) &request, sizeof(update_request_t));
	request.id = REQUEST_ID; // setup our future requests to update id 0 and value 0
	request.value = DUMMY;

	printf("Starting in Pedestrian state!\n");
	sleep(1);
	state = PEDESTRIAN;
	generate_outputs();
}

int get_state(void) {
	return state;
}

static void change_state(int transition) {
	// printing Maintenance entry/exit and train arriving/clearing
	switch (transition) {
		case M_SW_HI:
			if (state != MAINTENANCE && state != M_TRAIN && state != M_CLR) { // error-checking
				printf("Maintenance entry!\n");
			}
			break;
		case M_SW_LO:
			if (state == MAINTENANCE || state == M_TRAIN || state == M_CLR) { // error-checking
				printf("Maintenance exit!\n");
				reset_ttc(); // clear the blue light maintenance counter
				blue_off();
			}
			break;
		case T_SW_HI:
			printf("Train is arriving!\n");
			if (state != MAINTENANCE && state != M_TRAIN && state != M_CLR && state != Y_TRAIN) {
				reset_ttc(); 	 				// clear the timer counter
			}
			break;
		case T_SW_LO:
			if (state == TRAIN || state == M_TRAIN || state == Y_TRAIN) { // error-checking
				printf("Train is clearing!\n");
			}
			break;
		default:
			break;
	}

	// next state logic
	int next_state = state;
	switch (state) {
		// general states
		case PEDESTRIAN:
			if (transition == T_INT) 		next_state = Y2G;
			break;
		case Y2G:
			if (transition == T_INT)		next_state = V_MIN;
			break;
		case Y2R:
			if (transition == T_INT) 		next_state = PEDESTRIAN;
			break;
		case V_MIN:
			if (transition == T_INT) 		next_state = V_OK;
			else if (transition == P_BTN) 	next_state = V_MIN_PED;
			break;
		case V_OK:
			if (transition == P_BTN) 		next_state = Y2R;
			break;
		case V_MIN_PED:
			if (transition == T_INT) 		next_state = Y2R;
			break;

		// train states
		case TRAIN:
			if (transition == M_SW_HI) 		next_state = M_TRAIN;
			else if (transition == T_SW_LO) next_state = PED_TRAIN;
			break;
		case Y_TRAIN:
			if (transition == M_SW_HI) 		next_state = M_TRAIN;
			else if (transition == T_INT) 	next_state = TRAIN;
			else if (transition == T_SW_LO) next_state = PED_TRAIN;
			break;
		case PED_TRAIN:
			if (transition == T_SW_HI) 		next_state = TRAIN;
			else if (transition == T_INT) 	next_state = Y2G;
			break;

		// maintenence states
		case MAINTENANCE:
			if (transition == T_SW_HI) 		next_state = M_TRAIN;
			else if (transition == M_SW_LO) next_state = PEDESTRIAN;
			break;
		case M_TRAIN:
			if (transition == T_SW_LO) 		next_state = M_CLR;
			else if (transition == M_SW_LO) next_state = TRAIN;
			break;
		case M_CLR:
			if (transition == T_SW_HI) 		next_state = M_TRAIN;
			else 							next_state = MAINTENANCE;
			break;
		default:
			break;
	}

	// train arriving 2nd highest precedence (ignoring if in MAINTENCE STATE)
	if (transition == T_SW_HI && state != MAINTENANCE && state != PED_TRAIN && state != TRAIN && state != M_TRAIN && state != M_CLR)
		next_state = Y_TRAIN;

	// maintenance highest precedence (ignoring if in TRAIN state)
	if (transition == M_SW_HI && state != TRAIN && state != Y_TRAIN && state != M_TRAIN && state != M_CLR)
		next_state = MAINTENANCE;

	if (transition == DONE) {
		state = DONE;
		return;
	}

	printf("curr state: %d, next state: %d, transition: %d\n", state, next_state, transition);

	// update state if needed
	if (next_state != state) {
		state = next_state;
		generate_outputs();
	}
}

static void generate_outputs(void) {
	// default outputs
	led_set(PED_LIGHT, LED_OFF);
	led6_set(OFF);

	switch (state) {
		// general states
		case PEDESTRIAN:
			// load timer w/ PED_TIME interrupt
			restart_ttc(PED_TIME);

			// set gate high
			servo_set(OPEN);

			// set PED light
			led_set(PED_LIGHT, LED_ON);

			// set RED light
			led6_set(R);
			break;
		case Y2G:
			// falls through to Y2R
		case Y2R:
			// load timer w/ LIGHT_TIME interrupt
			restart_ttc(LIGHT_TIME);

			// set gate high
			servo_set(OPEN);

			// set YELLOW light
			led6_set(Y);
			break;
		case V_MIN:
			// load timer w// V_MIN_TIME interrupt
			restart_ttc(V_MIN_TIME);
			// set GREEN light falls through
		case V_OK:
			// set GREEN light falls through
		case V_MIN_PED:
			// set GREEN light
			led6_set(G);
			// set gate high
			servo_set(OPEN);
			break;

		// train states
		case TRAIN:
			// close gate and print
			servo_set(CLOSED);
			printf("Gate is closed!\n");
			// RED light
			led6_set(R);
			// PED light
			led_set(PED_LIGHT, LED_ON);
			break;
		case Y_TRAIN:
			// load timer w/ LIGHT_TIME interrupt
			restart_ttc(LIGHT_TIME);
			// set gate high
			servo_set(OPEN);
			// YELLOW light
			led6_set(Y);
			break;
		case PED_TRAIN:
			// load timer w/ PED_TIME interrupt
			restart_ttc(PED_TIME);
			// RED light
			led6_set(R);
			// PED light
			led_set(PED_LIGHT, LED_ON);
			// open gate and print
			servo_set(OPEN);
			printf("Gate is open!\n");
			break;

		// maintenance states
		case MAINTENANCE:
			// same as M_TRAIN
		case M_TRAIN:
			// BLUE light on
			blueStatus = LED_ON;
			led6_set(B);
			// load timer for 1 sec freq
			restart_ttc(BLUE_TIME);
			break;
		case M_CLR:
			change_state(DEFAULT);
			break;
		default:
			break;
	}
}
