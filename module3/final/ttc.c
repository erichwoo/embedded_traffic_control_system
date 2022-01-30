
#include "ttc.h"

static XTtcPs ttcportPs;

static void (*saved_ttc_callback)(void);

static void ttc_handler(void *devicep) {
	/* coerce the generic pointer into a ttc */
	XTtcPs *dev = (XTtcPs*)devicep;
	saved_ttc_callback();

	// use the status returned by this dev to clear the interrupt on it
	XTtcPs_ClearInterruptStatus(dev, XTtcPs_GetInterruptStatus(dev));
}

/*
 * ttc_init -- initialize the ttc freqency and callback
 */
void ttc_init(u32 freq, void (*ttc_callback)(void)) {
	saved_ttc_callback = ttc_callback;

	// initialize the TTC and immediately disable interrupts
	XTtcPs_CfgInitialize(&ttcportPs, XTtcPs_LookupConfig(XPAR_XTTCPS_0_DEVICE_ID), XPAR_XTTCPS_0_BASEADDR); // XPAR_PS7_GPIO_0_BASEADDR
	XTtcPs_SetOptions(&ttcportPs, XTTCPS_OPTION_INTERVAL_MODE);

	XInterval *interval = NULL;
	u8 *prescaler = NULL;
	XTtcPs_CalcIntervalFromFreq(&ttcportPs, freq, interval, prescaler);
	XTtcPs_SetPrescaler(&ttcportPs, *prescaler);
	XTtcPs_SetInterval(&ttcportPs, *interval);

	XTtcPs_DisableInterrupts(&ttcportPs, XTTCPS_IXR_INTERVAL_MASK);

	/* connect handler to the gic (c.f. gic.h) */
	gic_connect(XPAR_XTTCPS_0_INTR, &ttc_handler, (void*) &ttcportPs);
}

/*
 * ttc_start -- start the ttc
 */
void ttc_start(void) {
	XTtcPs_EnableInterrupts(&ttcportPs, XTTCPS_IXR_INTERVAL_MASK);
	XTtcPs_Start(&ttcportPs);
}

/*
 * ttc_stop -- stop the ttc
 */
void ttc_stop(void) {
	XTtcPs_Stop(&ttcportPs);
	XTtcPs_DisableInterrupts(&ttcportPs, XTTCPS_IXR_INTERVAL_MASK);
}

/*
 * ttc_close -- close down the ttc
 */
void ttc_close(void) {
	XTtcPs_DisableInterrupts(&ttcportPs, XTTCPS_IXR_INTERVAL_MASK);
	gic_disconnect(XPAR_XTTCPS_0_INTR);
}
