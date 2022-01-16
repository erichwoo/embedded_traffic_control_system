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
#include "gic.h"		/* interrupt controller interface */
#include "xgpio.h"		/* axi gpio interface */

/* hidden private state */
static XGpio btnport;	       /* btn GPIO port instance */
static int pushes=0;	       /* variable used to count interrupts */

#define INPUT 1
#define CHANNEL1 1
/*
 * control is passed to this function when a button is pushed
 *
 * devicep -- ptr to the device that caused the interrupt
 */
void btn_handler(void *devicep) {
	/* coerce the generic pointer into a gpio */
	XGpio *dev = (XGpio*)devicep;

	u32 portStatus = XGpio_DiscreteRead(&btnport, CHANNEL1) & 0xF;
	/* Read buttons for high, then act on it */
	if(portStatus > 0){
		// which button was pressed?
		int led = 0;
		while(portStatus != 1){
			portStatus >>= 1;
			led++;
		}
		printf("which led to toggle: %d\n", led);
		led_toggle(led);

		pushes++;
		printf(".");
	}

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

  // initialize LED module
  led_init();

  printf("[hello]\n"); /* so we are know its alive */
  pushes=0;
  while(pushes<5) /* do nothing and handle interrupts */
	  ;

  printf("\n[done]\n");

  /* disconnect the interrupts (c.f. gic.h) */
  gic_disconnect(XPAR_FABRIC_GPIO_1_VEC_ID);

  /* close the gic (c.f. gic.h)*/
  gic_close();

  printf("gic closed\n");
  cleanup_platform();					/* cleanup the hardware platform */
  return 0;
}

