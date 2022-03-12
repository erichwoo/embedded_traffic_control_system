
#include "fsm.h"

static void load_timer(int interrupt);

static void change_state(int transition);
static void generate_outputs(void);

static int counter = 0;
static int trigger;
static int state;
static bool blueStatus = LED_OFF;

static void ttc_clear(void) {
	counter = 0;
	ttc_stop();
	ttc_reset();

	// turn off
	led6_set(OFF);
	blueStatus = LED_OFF;
}

void ttc_callback(void) {
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
			ttc_clear();
			// change state
			change_state(T_INT);
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

int get_state(void) {
	return state;
}

void init_state(void) {
	printf("Starting in Pedestrian state!\n");
	sleep(1);
	state = PEDESTRIAN;
	generate_outputs();
}

static void change_state(int transition) {
	printf("curr state: %d, transition: %d\n", state, transition);

	// printing Maintenance entry/exit and train arriving/clearing
	switch (transition) {
		case M_SW_HI:
			printf("Maintenance entry!\n");
			break;
		case M_SW_LO:
			if (state == MAINTENANCE || state == M_TRAIN || state == M_CLR) { // error-checking
				printf("Maintenance exit!\n");
				ttc_clear(); // clear the blue light maintenance counter
			}
			break;
		case T_SW_HI:
			printf("Train is arriving!\n");
			ttc_clear(); 	 // clear the timer counter
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
			if (transition == T_INT) next_state = Y2G;
			break;
		case Y2G:
			if (transition == T_INT) next_state = V_MIN;
			break;
		case Y2R:
			if (transition == T_INT) next_state = PEDESTRIAN;
			break;
		case V_MIN:
			if (transition == T_INT) next_state = V_OK;
			else if (transition == P_BTN) next_state = V_MIN_PED;
			break;
		case V_OK:
			if (transition == P_BTN) next_state = Y2R;
			break;
		case V_MIN_PED:
			if (transition == T_INT) next_state = Y2R;
			break;

		// train states
		case TRAIN:
			if (transition == M_SW_HI) next_state = M_TRAIN;
			else if (transition == T_SW_LO) next_state = PED_TRAIN;
			break;
		case Y_TRAIN:
			if (transition == M_SW_HI) next_state = M_TRAIN;
			else if (transition == T_INT) next_state = TRAIN;
			else if (transition == T_SW_LO) next_state = PED_TRAIN;
			break;
		case PED_TRAIN:
			if (transition == T_SW_HI) next_state = TRAIN;
			else if (transition == T_INT) next_state = Y2G;
			break;

		// maintenence states
		case MAINTENANCE:
			if (transition == T_SW_HI) next_state = M_TRAIN;
			else if (transition == M_SW_LO) next_state = PEDESTRIAN;
			break;
		case M_TRAIN:
			if (transition == T_SW_LO) next_state = M_CLR;
			else if (transition == M_SW_LO) next_state = TRAIN;
			break;
		case M_CLR:
			next_state = MAINTENANCE;
			break;
		default:
			break;
	}

	// train arriving 2nd highest precedence (ignoring if in MAINTENCE STATE)
	if (transition == T_SW_HI && state != MAINTENANCE && state != PED_TRAIN)
		next_state = Y_TRAIN;

	// maintenance highest precedence (ignoring if in TRAIN state)
	if (transition == M_SW_HI && state != TRAIN && state != Y_TRAIN)
		next_state = MAINTENANCE;

	if (transition == DONE) {
		state = DONE;
		return;
	}

	// update state if needed
	if (next_state != state) {
		state = next_state;
		printf("next state is: %d\n", state);
		generate_outputs();
	}
}

static void load_timer(int interrupt) {
	trigger = interrupt;
	ttc_start();
}

static void generate_outputs(void) {
	// default outputs
	led_set(PED_LIGHT, LED_OFF);
	led6_set(OFF);
		// switch gate high

	switch (state) {
		// general states
		case PEDESTRIAN:
			// load timer w/ PED_TIME interrupt
			load_timer(PED_TIME);

			// set PED light
			led_set(PED_LIGHT, LED_ON);

			// set RED light
			led6_set(R);
			break;
		case Y2G:
			// falls through to Y2R
		case Y2R:
			// load timer w/ LIGHT_TIME interrupt
			load_timer(LIGHT_TIME);

			// set YELLOW light
			led6_set(Y);
			break;
		case V_MIN:
			// load timer w// V_MIN_TIME interrupt
			load_timer(V_MIN_TIME);
			// set GREEN light falls through
		case V_OK:
			// set GREEN light falls through
		case V_MIN_PED:
			// set GREEN light
			led6_set(G);
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
			load_timer(LIGHT_TIME);
			// YELLOW light
			led6_set(Y);
			break;
		case PED_TRAIN:
			// load timer w/ PED_TIME interrupt
			load_timer(PED_TIME);
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
			load_timer(BLUE_TIME);
			break;
		case M_CLR:
			change_state(DEFAULT);
			break;
		default:
			break;
	}
}
