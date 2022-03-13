/*
 * Header file for traffic_wrapper.c
 */

#pragma once

#include <stdbool.h>    /* bool */
#include "led.h"		/* LED Module for traffic/blue/ped lights */
#include "servo.h"		/* servo module for gate */
#include "adc.h"		/* adc module for potentiometer */

// gate position
#define OPEN 	SERVO_MAX
#define CLOSED 	SERVO_MIN

// ped light
#define PED_LIGHT	4

// traffic lights (including blue light);
void set_traffic_light(u32 color);
void close_traffic_light(void);
void set_blue_light(bool on_off);
void set_ped_light(bool on_off);

// gate operation
void open_gate(void);
void close_gate(void);
void manual_gate(void);
