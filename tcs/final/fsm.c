/*
 * fsm.c -- Event-Driven Finite State Machine for Traffic Control System
 *
 */

#include "fsm.h"

#define UPDATE_BYTE_COUNT sizeof(update_response_t)

#define FETCH_FREQ 3						/* how often to poll server for train status, per 100ms */

/************************ STATIC FUNCTION DECLARATIONS ***********************/

static void set_blue(bool on_off);
static void reset_ttc(void);
static void restart_ttc(int trig);
static void change_state(int transition);
static void generate_outputs(void);

/****************************** STATIC VARIABLES *****************************/

static bool ttcIsOn = false;		/* if we are using ttc for FSM (rather than just polling uart0) */
static int counter = 0;             /* counter/divider for ttc, which operates at 10 Hz */
static int trigger;				    /* desired trigger (sec) to run new code */
static int state;					/* current FSM state */
static bool blueStatus = LED_OFF;   /* LED6 Blue-light status (On/Off) */

// uart0 interfacing
static int fetchCounter = 0; 		/* fetch every 300ms > 250ms */
static update_request_t request = {UPDATE, SERVER_ID, SERVER_START_VAL};
static update_response_t response;
static u8 responseCount = 0;
static int remoteTrans;

static bool init = true;

/**/

static void set_blue(bool on_off) {
	blueStatus = on_off;
	set_blue_light(blueStatus);
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
	// poll Wifi Module # 300ms intervals (counter increments @ 100ms intervals)
	fetchCounter++;
	if (fetchCounter >= FETCH_FREQ) {
		uart_send(WIFI_DEV, (void*) &request, sizeof(update_request_t));
		fetchCounter = 0;
	}

	// FSM
	if (ttcIsOn) {
		counter++;

		// polling potentiometer if in MAINTENANCE STATES
		if (M_STATES) {
			manual_gate();
		}

		if (counter >= trigger*10) {
			if (M_STATES) {
				//restart the counter
				counter = 0;

				// toggle blue
				set_blue(!blueStatus);
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

	// if response is fully stored, analyze value @ SERVER_ID for potential transition
	if (responseCount == UPDATE_BYTE_COUNT) {
		int newTrans = response.values[SERVER_ID];

		// deal with server response
		if (init) {
			remoteTrans = newTrans;
			init = false;
		}
		else {
			if (newTrans >= M_SW_HI && newTrans <= T_SW_LO && newTrans != remoteTrans) {
				remoteTrans = newTrans;
				change_state(newTrans);
			}
			else remoteTrans = newTrans;
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
	// path to exit program
	if (transition == DONE) {
		state = DONE;
		return;
	}

	// printing Maintenance entry/exit and train arriving/clearing
	switch (transition) {
		case M_SW_HI:
			if (!M_STATES) { // error-checking
				printf("Maintenance entry!\n");
			}
			break;
		case M_SW_LO:
			if (M_STATES) { // error-checking
				printf("Maintenance exit!\n");
				reset_ttc(); 				// clear the blue light maintenance counter as we leave MAINTENANCE
				set_blue(LED_OFF);
			}
			break;
		case T_SW_HI:
			printf("Train is arriving!\n");
			if (!M_STATES && state != Y_TRAIN) {
				reset_ttc(); 	 				// clear the timer counter
			}
			break;
		case T_SW_LO:
			if (T_STATES) { // error-checking
				printf("Train is clearing!\n");
			}
			break;
		default:
			break;
	}

	// next state logic
	int next_state = state;
	switch (state) {
		/**************************** GENERAL STATES ***************************/
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

		/**************************** TRAIN STATES ***************************/
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

		/*********************** MAINTENANCE STATES ************************/
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

	// train arriving 2nd highest precedence (ignoring if in MAINTENANCE STATE or PED_TRAIN/TRAIN)
	if (transition == T_SW_HI && !M_STATES && state != PED_TRAIN && state != TRAIN)
		next_state = Y_TRAIN;

	// maintenance highest precedence (ignoring if in TRAIN or MAINTENANCE state)
	if (transition == M_SW_HI && !T_STATES && !M_STATES)
		next_state = MAINTENANCE;

	/***************************** GENERATE OUTPUTS FOR NEXT STATE *****************************/
	printf("curr state: %d, next state: %d, transition: %d\n", state, next_state, transition);
	if (next_state != state) {
		state = next_state;
		generate_outputs();
	}
}

static void generate_outputs(void) {
	// default outputs (PED light off, Traffic lights off)
	set_ped_light(LED_OFF);
	close_traffic_light();

	switch (state) {
		/************************** GENERAL STATES *****************************/
		case PEDESTRIAN: 				// open gate, set PED light, set RED light, load PED_TIME timer trigger
			restart_ttc(PED_TIME);
			open_gate();
			set_ped_light(LED_ON);
			set_traffic_light(R);
			break;
		case Y2G: 						// same outputs as Y2R
		case Y2R:						// open gate, set YELLOW light, load LIGHT_TIME timer trigger
			restart_ttc(LIGHT_TIME);
			open_gate();
			set_traffic_light(Y);
			break;
		case V_MIN:						// load V_MIN_TIME timer trigger + same outputs as V_OK/V_MIN_PED
			restart_ttc(V_MIN_TIME);
		case V_OK:						// same outputs as V_MIN_PED
		case V_MIN_PED:					// open gate, set GREEN light
			open_gate();
			set_traffic_light(G);
			break;

		/***************************** TRAIN STATES *********************************/
		case Y_TRAIN:					// open gate, set YELLOW light, load LIGHT_TIME timer trigger
			restart_ttc(LIGHT_TIME);
			open_gate();
			set_traffic_light(Y);
			break;
		case TRAIN:						// close gate & print, set PED light, set RED light
			close_gate();
			printf("Gate is closed!\n");
			set_ped_light(LED_ON);
			set_traffic_light(R);
			break;
		case PED_TRAIN:					// open gate, set PED light, set RED light, load PED_TIME timer trigger
			restart_ttc(PED_TIME);
			open_gate();
			printf("Gate is open!\n");
			set_ped_light(LED_ON);
			set_traffic_light(R);
			break;

		/************************** MAINTENANCE STATES *********************************/
		case MAINTENANCE:				// same outputs as M_TRAIN
		case M_TRAIN:					// set BLUE light, load BLUE_TIME timer trigger (for toggling)
			set_blue(LED_ON);
			restart_ttc(BLUE_TIME);
			break;
		case M_CLR:						// no outputs, immediately change state on default transition
			change_state(DEFAULT);
			break;
		default:
			break;
	}
}
