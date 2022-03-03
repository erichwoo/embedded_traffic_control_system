/*
 * ttc.h
 *
 * NOTE: The TTC hardware must be enabled (Timer 0 on the processing system) before it can be used!!
 *
 */
#pragma once

#include <stdio.h>
#include "xttcps.h"
#include "xparameters.h"  	/* constants used by the hardware */
#include "xil_types.h"		/* types used by xilinx */
#include "gic.h"
/*
 * ttc_init -- initialize the ttc freqency and callback
 */
void ttc_init(u32 freq, void (*ttc_callback)(void));

/*
 * ttc_set_freq -- set the frequency (Hz) of the ttc
 */
void ttc_set_freq(u32 freq);

/*
 * ttc_start -- start the ttc
 * simultaneously enables ttc interrupts
 */
void ttc_start(void);

/*
 * ttc_stop -- stop the ttc
 * simultaneously disables ttc interrupts
 */
void ttc_stop(void);

/*
 * ttc_close -- close down the ttc
 * simultaneously disables ttc interrupts
 */
void ttc_close(void);
