
#include "fsm.h"

static void load_timer(int interrupt);

static void change_state(int transition);
static void generate_outputs(void);

static int counter = 0;
static int trigger;
static int state;

void ttc_callback(void) {
	counter++;
	// polling potentiometer if in MAITNENANCE STATES
	if (counter >= trigger*10) {
		// reset counter before changing state
		counter = 0;
		ttc_stop();
		ttc_reset();

		// change state
		change_state(T_INT);
	}
}

void btn_callback(u32 btn) {
	if (btn == 3)
		change_state(DONE);
	else if (btn == 0 || btn == 1)
		change_state(P_BTN);
}

void sw_callback(u32 sw) {
	u32 val = (1 << sw) & io_sw_read();
	if (sw == 0 && val == 1) change_state(M_SW_HI);
	else if (sw == 0 && val == 0) change_state(M_SW_LO);
	else if (sw == 1 && val == 1) change_state(T_SW_HI);
	else if (sw == 1 && val == 0) change_state(T_SW_LO);
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
		case Y_TRAIN:
		case PED_TRAIN:
		case Y_TRAIN_CLR:

		// maintenence states
		case MAINTENANCE:
		case M_TRAIN:
		case M_CLR:
		default:
			break;
	}
	// train arriving 2nd highest precedence (ignoring if in MAINTENCE STATE)
	if (transition == T_SW_HI && state != MAINTENANCE)
		next_state = Y_TRAIN;

	// maintenance highest precedence (ignoring if in TRAIN state)
	if (transition == M_SW_HI && state != TRAIN)
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
		case Y_TRAIN:
		case PED_TRAIN:
		case Y_TRAIN_CLR:

		// maintenance states
		case MAINTENANCE:
		case M_TRAIN:
		case M_CLR:
		default:
			break;
	}
}
