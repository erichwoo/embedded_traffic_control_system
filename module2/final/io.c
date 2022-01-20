/**
 * Implementation of io.h file
 * Used for buttons and switches of module 2
 */

#include "io.h"

// callback functions for btn and sw
static void (*saved_btn_callback)(u32 btn);
static void (*saved_sw_callback)(u32 btn);

// the button/switch port XGpio reference
static XGpio btnport;		/* btn GPIO port instance */
static XGpio swport;		/* sw GPIO port instance */

/* hidden private state */
static u32 prevSwStates;		/* keep track of previous state of switch port (gpio dev 2) */

/* useful definitions */
#define INPUT 1				/* Set direction of GPIO Port pins */
#define CHANNEL1 1			/* which channel of GPIO device */

/******************************* STATIC FUNCTIONS ***********************************/

/*
 * Gets position of MS set bit, 0-indexed
 *
 */
static u32 oneHotDecoder(u32 portData){
	u32 led = 0;
	while(portData != 1){
		portData >>= 1;
		led++;
	}
	return led;
}

/*
 * control is passed to this function when a button is pushed
 *
 * devicep -- ptr to the device that caused the interrupt
 */
static void btn_handler(void *devicep) {
	/* coerce the generic pointer into a gpio */
	XGpio *dev = (XGpio*)devicep;

	u32 portStatus = XGpio_DiscreteRead(dev, CHANNEL1) & 0xF;
	/* Read buttons for high, then act on it */
	if(portStatus > 0){
		// which button was pressed?
		saved_btn_callback(oneHotDecoder(portStatus));
	}

	// always clear interrupt after handling it
	XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);
}

/*
 * control is passed to this function when a switch is flipped
 *
 * devicep -- ptr to the device that caused the interrupt
 */
static void sw_handler(void *devicep) {
	/* coerce the generic pointer into a gpio */
	XGpio *dev = (XGpio*)devicep;

	u32 portStatus = XGpio_DiscreteRead(dev, CHANNEL1) & 0xF;
	u32 diff = portStatus ^ prevSwStates;

	if(diff > 0){
		// which switch was toggled?
		saved_sw_callback(oneHotDecoder(diff));
	}

	// update prev switch states
	prevSwStates = portStatus;

	// always clear interrupt after handling it
	XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);
}

/******************************* MODULE FUNCTIONS **********************************/

/*
 * initialize the btns providing a callback
 */
void io_btn_init(void (*btn_callback)(u32 btn)){
	saved_btn_callback = btn_callback;

	/* initialize btnport (c.f. module 1) and immediately disable interrupts */
	XGpio_Initialize(&btnport, XPAR_AXI_GPIO_1_DEVICE_ID);
	XGpio_SetDataDirection(&btnport, CHANNEL1, INPUT);

	XGpio_InterruptDisable(&btnport, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalDisable(&btnport);

	/* connect handler to the gic (c.f. gic.h) */
	gic_connect(XPAR_FABRIC_GPIO_1_VEC_ID, &btn_handler, (void*) &btnport);

	/* enable interrupts on channel (c.f. table 2.1) */
	XGpio_InterruptEnable(&btnport, XGPIO_IR_CH1_MASK);
	/* enable interrupt to processor (c.f. table 2.1) */
	XGpio_InterruptGlobalEnable(&btnport);
}

/*
 * close the btns
 */
void io_btn_close(void){
	// disconnect the interrupts for gpio device 1 (aka buttons for module 2)
	gic_disconnect(XPAR_FABRIC_GPIO_1_VEC_ID);
}

/*
 * initialize the switches providing a callback
 */
void io_sw_init(void (*sw_callback)(u32 sw)){
	// save the sw callback
	saved_sw_callback = sw_callback;

	/* initialize swport and immediately disable interrupts */
	XGpio_Initialize(&swport, XPAR_AXI_GPIO_2_DEVICE_ID);
	XGpio_SetDataDirection(&swport, CHANNEL1, INPUT);

	XGpio_InterruptDisable(&swport, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalDisable(&swport);

	/* connect handler to the gic (c.f. gic.h) */
	gic_connect(XPAR_FABRIC_GPIO_2_VEC_ID, &sw_handler, (void*) &swport);

	/* enable interrupts on channel (c.f. table 2.1) */
	XGpio_InterruptEnable(&swport, XGPIO_IR_CH1_MASK);
	/* enable interrupt to processor (c.f. table 2.1) */
	XGpio_InterruptGlobalEnable(&swport);

	// Set the initial sw state for sw handler
	prevSwStates = XGpio_DiscreteRead(&swport, CHANNEL1) & 0xF;
}

/*
 * close the switches
 */
void io_sw_close(void){
	// disconnect the interrupts for gpio device 2 (aka switches for module 2)
	gic_disconnect(XPAR_FABRIC_GPIO_2_VEC_ID);
}
