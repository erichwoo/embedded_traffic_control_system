/**
 * Implements the adc.h module.
 */

#include "adc.h"

/* Experimentally observed is 63000, but we will round to 2 decimal places, so we have .01 tolerance*/
#define MAXADC 62700.0f

/* ADC device onboard the ps */
static XAdcPs XADCPortPs;

/*
 * initialize the adc module
 */
void adc_init(void){
	// initialize configuration of the internal XADC
	XAdcPs_CfgInitialize(&XADCPortPs, XAdcPs_LookupConfig(XPAR_XADCPS_0_DEVICE_ID), XPAR_XADCPS_0_BASEADDR);
	XAdcPs_SetSequencerMode(&XADCPortPs, XADCPS_SEQ_MODE_SAFE);
	XAdcPs_SetAlarmEnables(&XADCPortPs, 0U);
	XAdcPs_SetSeqChEnables(&XADCPortPs, XADCPS_SEQ_CH_TEMP | XADCPS_SEQ_CH_VCCINT | XADCPS_SEQ_CH_AUX14);
	XAdcPs_SetSequencerMode(&XADCPortPs, XADCPS_SEQ_MODE_CONTINPASS);
}

/*
 * get the internal temperature in degree's celsius
 */
float adc_get_temp(void){
	// grab the raw ADC data, pass it to the built in raw to temperature function
	u16 rawData = XAdcPs_GetAdcData(&XADCPortPs, XADCPS_CH_TEMP);
	return XAdcPs_RawToTemperature(rawData);
}

/*
 * get the internal vcc voltage (should be ~1.0v)
 */
float adc_get_vccint(void){
	// reading internal signals uses a 3.0V reference (it's actually 2^16 buckets)
	u16 rawData = XAdcPs_GetAdcData(&XADCPortPs, XADCPS_CH_VCCINT);
	return XAdcPs_RawToVoltage(rawData);
}

/*
 * get the **corrected** potentiometer voltage (should be between 0 and 1v)
 */
float adc_get_pot(void){
	// reading external signals uses a 1.0V reference (can't use built-in func)
	u16 rawData = XAdcPs_GetAdcData(&XADCPortPs, XADCPS_AUX14_OFFSET);
	return ((float)(rawData))/MAXADC;
}

u32 adc_get_pot_percent(void) {
	return (u32)(adc_get_pot()*100);
}
