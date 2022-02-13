/**
 * uart.h - UART Module header file
 */

#include <stdio.h>
#include "xuartps.h"
#include "xparameters.h"  	/* constants used by the hardware */
#include "xil_types.h"		/* types used by xilinx */
#include "gic.h"

/* Defines */
#define CONFIGURE 0
#define PING 1
#define UPDATE 2
#define DONE 3

extern u8 mode;

void uart_init(void);

void uart_close(void);
