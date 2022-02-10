#include "uart.h"

XUartPs uart1;

static void uart_handler(void *CallBackRef, u32 Event, unsigned int EventData);

void uart_init(void) {
	XUartPs_CfgInitialize(&uart1, XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID), XPAR_PS7_UART_1_BASEADDR);
	XUartPs_DisableUart(&uart1);
	XUartPs_SetBaudRate(&uart1); 			//Sets the baud rate for the device
	XUartPs_SetFifoThreshold(&uart1); 	//Sets the FIFO trigger threashold
	XUartPs_SetInterruptMask(&uart1, XPAR_XUARTPS_1_INTR); 	//Sets the interrupt mask
	XUartPs_SetHandler(&uart1, &uart_handler, (void*) &uart1);

	gic_connect(XPAR_PS7_UART_1_DEVICE_ID, &XUartPs_InterruptHandler, (void*) &uart1);
}

void uart_close(void) {
	gic_disconnect(XPAR_PS7_UART_1_DEVICE_ID);
}
