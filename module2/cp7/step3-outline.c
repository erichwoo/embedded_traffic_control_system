/*
 * main.c -- A program to print a dot each time button 0 is pressed.
 *
 *  Some useful values:
 *  -- XPAR_AXI_GPIO_1_DEVICE_ID -- xparameters.h
 *  -- XPAR_FABRIC_GPIO_1_VEC_ID -- xparameters.h
 *  -- XGPIO_IR_CH1_MASK         -- xgpio_l.h (included by xgpio.h)
 */
#include <stdio.h>		/* getchar,printf */
#include <stdlib.h>		/* strtod */
#include <stdbool.h>	/* type bool */
#include <unistd.h>		/* sleep */
#include <string.h>

#include "platform.h"		/* ZYBO board interface */
#include "xil_types.h"		/* u32, s32 etc */
#include "xparameters.h"	/* constants used by hardware */

#include "led.h"		/* LED Module */
#include "io.h"			/* io module */
#include "gic.h"		/* interrupt controller interface */
#include "xgpio.h"		/* axi gpio interface */

/* hidden private state */
static int pushes=0;	       /* variable used to count interrupts */
static u32 prevSwStates = 0;		/* keep track of previous state of switch port (gpio dev 2) */

/* function signatures */
u32 oneHotDecoder(u32 portData);

/*
 * control is passed to this function when a button is pushed
 *
 * devicep -- ptr to the device that caused the interrupt
 */
void btn_handler(void *devicep) {
	/* coerce the generic pointer into a gpio */
	XGpio *dev = (XGpio*)devicep;

	u32 portStatus = XGpio_DiscreteRead(dev, CHANNEL1) & 0xF;
	/* Read buttons for high, then act on it */
	if(portStatus > 0){
		// which button was pressed?
		led_toggle(oneHotDecoder(portStatus));

		pushes++;
		printf(".");
	}

	// always clear interrupt after handling it
	XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);
	fflush(stdout);
}

/*
 * control is passed to this function when a switch is flipped
 *
 * devicep -- ptr to the device that caused the interrupt
 */
void sw_handler(void *devicep) {
	/* coerce the generic pointer into a gpio */
	XGpio *dev = (XGpio*)devicep;

	u32 portStatus = XGpio_DiscreteRead(dev, CHANNEL1) & 0xF;
	u32 diff = portStatus ^ prevSwStates;

	if(diff > 0){
		// which switch was toggled?
		led_toggle(oneHotDecoder(diff));
	}

	// update prev switch states
	prevSwStates = portStatus;

	// always clear interrupt after handling it
	XGpio_InterruptClear(dev, XGPIO_IR_CH1_MASK);
	fflush(stdout);
}


int main() {
  init_platform();				

  /* initialize the gic (c.f. gic.h) */
  if (gic_init() == XST_FAILURE) {
	  printf("Failed to initialize the gic. Exiting...\n");
	  return -1;
  }

  // initialize the buttons using io module
  io_btn_init(&btn_handler);
  io_sw_init(&sw_handler);

  // initialize LED module
  led_init();

  printf("[hello]\n"); /* so we are know its alive */
  pushes=0;
  while(pushes<5) /* do nothing and handle interrupts */
	  ;

  printf("\n[done]\n");


  /* close the gic (c.f. gic.h)*/
  io_sw_close();
  io_btn_close();
  gic_close();

  printf("gic closed\n");
  cleanup_platform();					/* cleanup the hardware platform */
  return 0;
}

u32 oneHotDecoder(u32 portData){
	u32 led = 0;
	while(portData != 1){
		portData >>= 1;
		led++;
	}
	return led;
}
