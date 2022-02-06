/**
 * Implements the adc.h module.
 */

#include "adc.h"

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
	// XAdcPs_SetSequencerMode(&XADCPortPs, XADCPS_SEQ_MODE_ONEPASS); to turn off? ask about this
	//TODO: check we selected the correct channel enables for the potentiometer
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
	//TODO: check correctness of internal voltage (shouldn't it be 3.3v?)
	u16 rawData = XAdcPs_GetAdcData(&XADCPortPs, XADCPS_CH_VCCINT);
	return XAdcPs_RawToVoltage(rawData);
}

/*
 * get the **corrected** potentiometer voltage (should be between 0 and 1v)
 */
float adc_get_pot(void){
	//TODO, incorrect voltage scaling, also check is this the right channel to read?
	u16 rawData = XAdcPs_GetAdcData(&XADCPortPs, XADCPS_AUX14_OFFSET);
	//TODO: do we need to do fancy math with our raw data or does this function work?
	return XAdcPs_RawToVoltage(rawData);
}
