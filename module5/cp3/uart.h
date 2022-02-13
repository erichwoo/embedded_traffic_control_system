/**
 * uart.h - UART Module header file
 */

#include <stdio.h>
#include "xuartps.h"
#include "xparameters.h"  	/* constants used by the hardware */
#include "xil_types.h"		/* types used by xilinx */
#include "gic.h"

void uart_init(void);

void uart_close(void);
