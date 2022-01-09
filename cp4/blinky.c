/*
 * blinky.c -- working with Serial I/O and GPIO
 *
 * Assumes the LED's are connected to AXI_GPIO_0, on channel 1
 *
 * Terminal Settings:
 *  -Baud: 115200
 *  -Data bits: 8
 *  -Parity: no
 *  -Stop bits: 1
 */
#include <stdio.h>							/* printf(), getchar() */
#include <stdlib.h>
#include <strings.h>
#include "xil_types.h"					/* u32, u16 etc */
#include "platform.h"						/* ZYBOboard interface */
#include <xgpio.h>							/* Xilinx GPIO functions */
#include "xparameters.h"				/* constants used by the hardware */

#define OUTPUT 0x0							/* setting GPIO direction to output */
#define CHANNEL1 1							/* channel 1 of the GPIO port */

int main() {
   XGpio port;									/* GPIO port connected to the leds */

   init_platform();							/* initialize the hardware platform */

	 /* 
		* set stdin unbuffered, forcing getchar to return immediately when
		* a character is typed.
		*/
	 setvbuf(stdin,NULL,_IONBF,0);
	 
	 printf("[Hello]\n");
	 
   XGpio_Initialize(&port, XPAR_AXI_GPIO_0_DEVICE_ID);	/* initialize device AXI_GPIO_0 */
   XGpio_SetDataDirection(&port, CHANNEL1, OUTPUT);	    /* set tristate buffer to output */
	 XGpio_DiscreteWrite(&port, CHANNEL1, 0x1);						/* turn on led0 */

	 const int len = 64;
	 char buffer[len];

	 // while input line is not "quit"
	 while (strcmp(buffer, "q") != 0) {
		 // start new input line
		 printf(">");

		 // initialize ptr to end of string/input
		 int i = 0;
		 buffer[i] = '\0';
		 char c;
		 // while character isn't newline, keep reading
		 while ( (c = getchar()) != '\r') {
			 printf("%c", c);  // echo back char

			 // write char to end of buffer
			 if(i >= len - 1){  // safety check, wrap back to beginning of buffer
			 	i = 0;
			 }
			 buffer[i++] = c;
			 buffer[i] = '\0';
		 }
		 printf("\n");

		 // check if single char && 0 - 3
		 long val = strtol(buffer, NULL, 10);
		 if (i == 1 && val >= 0 && val <= 3) {
			 printf("[%s]\n", buffer);
		 }
	 }

	 printf("---- program exits here ----\n");

   cleanup_platform();					/* cleanup the hardware platform */
   return 0;
}
