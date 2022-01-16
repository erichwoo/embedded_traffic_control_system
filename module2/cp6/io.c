/**
 * Implementation of io.h file
 * Used for buttons and switches of module 2
 */

#include "io.h"

// button callback function
static void (*saved_btn_callback)(void* btnAddress);
// the button port XGpio reference
static XGpio btnport;		/* btn GPIO port instance */

/*
 * initialize the btns providing a callback
 */
void io_btn_init(void (*btn_callback)(void* btnAddress)){
	saved_btn_callback = btn_callback;

	/* initialize btnport (c.f. module 1) and immediately disable interrupts */
	XGpio_Initialize(&btnport, XPAR_AXI_GPIO_1_DEVICE_ID);
	XGpio_SetDataDirection(&btnport, CHANNEL1, INPUT);

	XGpio_InterruptDisable(&btnport, XGPIO_IR_CH1_MASK);
	XGpio_InterruptGlobalDisable(&btnport);

	/* connect handler to the gic (c.f. gic.h) */
	gic_connect(XPAR_FABRIC_GPIO_1_VEC_ID, saved_btn_callback, (void*) &btnport);

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
void io_sw_init(void (*sw_callback)(u32 sw));

/*
 * close the switches
 */
void io_sw_close(void);
