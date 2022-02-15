/**
 * wifi.h - UART Module header file
 * In reality, this module is controlling a lot more than just the UART, as
 * it is really a device driver for the Wi-Fly module, what with handling
 * the mode that we're in (UI-related), as well as communication with the
 * Wi-Fly peripheral over USART and handling all communication.
 */

#include <stdio.h>
#include "xuartps.h"
#include "xparameters.h"  	/* constants used by the hardware */
#include "xil_types.h"		/* types used by xilinx */
#include "gic.h"

/* Defines */
// the modes the user can be in
#define CONFIGURE 0
#define PING 1
#define UPDATE 2
#define DONE 3

// the current mode (state), controlled
extern u8 mode;


typedef struct {
	int type;	// must be assigned to PING
	int id;		// must be assigned to your id
} ping_t;

typedef struct {
int type; 	/* must be assigned to UPDATE */
int id;		/* must be assigned to your id */
int value; 	/* must be assigned to some value */
} update_request_t;

typedef struct {
int type;
int id;
int average;
int values[30];
} update_response_t;

void uart_init(void);

void uart_close(void);

void uart_send(u8 dev, void* addr, u32 size);
