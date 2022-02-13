/**
 * Implementing the header file uart.h
 */

#include "uart.h"

// DEFINES
#define TRIG_LEVEL 1
#define UART0_BAUD 9600
#define UART1_BAUD XUARTPS_DFT_BAUDRATE


// UART Devices
static XUartPs uart0;
static XUartPs uart1;

static void uart0_handler(void *CallBackRef, u32 Event, unsigned int EventData){
	// for loopback, correctly determine src and destination devices
	XUartPs* src = (XUartPs*) CallBackRef;
	XUartPs* dest = &uart1;

	if(Event == XUARTPS_EVENT_RECV_DATA && src == &uart0){
		u8 buffer;
		XUartPs_Recv(src, &buffer, TRIG_LEVEL);
		XUartPs_Send(dest, &buffer, TRIG_LEVEL);

		if(buffer == (u8) '\r'){
			u8 newLine = (u8) '\n';
			XUartPs_Send(dest, &newLine, TRIG_LEVEL);
		}
	}
}

static void uart1_handler(void *CallBackRef, u32 Event, unsigned int EventData){
	XUartPs* src = (XUartPs*) CallBackRef;
	XUartPs* dest = &uart0;

	if(Event == XUARTPS_EVENT_RECV_DATA && src == &uart1){
		u8 buffer;
		XUartPs_Recv(src, &buffer, TRIG_LEVEL);
		XUartPs_Send(dest, &buffer, TRIG_LEVEL);
	}
}

void uart_init(void) {
	// UART 0
	XUartPs_CfgInitialize(&uart0, XUartPs_LookupConfig(XPAR_PS7_UART_0_DEVICE_ID), XPAR_PS7_UART_0_BASEADDR);
	XUartPs_DisableUart(&uart0);
	XUartPs_SetBaudRate(&uart0, UART0_BAUD);		//Sets the baud rate for the device
	XUartPs_SetFifoThreshold(&uart0, TRIG_LEVEL); 			//Sets the FIFO trigger threashold
	XUartPs_SetInterruptMask(&uart0, XUARTPS_IXR_RXOVR);	//Sets the interrupt mask
	XUartPs_SetHandler(&uart0, (XUartPs_Handler) uart0_handler, (void*) &uart0);

	gic_connect(XPAR_XUARTPS_0_INTR, (Xil_InterruptHandler) XUartPs_InterruptHandler, (void*) &uart0);

	// UART 1
	XUartPs_CfgInitialize(&uart1, XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID), XPAR_PS7_UART_1_BASEADDR);
	XUartPs_DisableUart(&uart1);
	XUartPs_SetBaudRate(&uart1, UART1_BAUD);		//Sets the baud rate for the device
	XUartPs_SetFifoThreshold(&uart1, TRIG_LEVEL); 			//Sets the FIFO trigger threashold
	XUartPs_SetInterruptMask(&uart1, XUARTPS_IXR_RXOVR);	//Sets the interrupt mask
	XUartPs_SetHandler(&uart1, (XUartPs_Handler) uart1_handler, (void*) &uart1);

	gic_connect(XPAR_XUARTPS_1_INTR, (Xil_InterruptHandler) XUartPs_InterruptHandler, (void*) &uart1);
}

void uart_close(void) {
	gic_disconnect(XPAR_XUARTPS_0_INTR);
	gic_disconnect(XPAR_XUARTPS_1_INTR);
}
