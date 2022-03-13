/*
 * Wrapper functions for Traffic Control Peripherals
 */

#include "traffic_wrapper.h"

// leds
void set_traffic_light(u32 color) {
	led6_set(color);
}

void close_traffic_light(void) {
	set_traffic_light(OFF);
}

void set_blue_light(bool on_off) {
	set_traffic_light((on_off) ? B : OFF);
}

void set_ped_light(bool on_off) {
	led_set(PED_LIGHT, on_off);
}

// gate operation
void open_gate(void) {
	servo_set(OPEN);
}
void close_gate(void) {
	servo_set(CLOSED);
}

void manual_gate(void) {
	servo_set_percent(adc_get_pot_percent());
}
