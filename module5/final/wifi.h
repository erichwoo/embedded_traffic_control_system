/**
 * wifi.h - UART Module header file
 * In reality, this module is controlling a lot more than just the UART, as
 * it is really a device driver for the Wi-Fly module, what with handling
 * the mode that we're in (UI-related), as well as communication with the
 * Wi-Fly peripheral over USART and handling all communication.
 */

#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include "xuartps.h"
#include "xparameters.h"  	/* constants used by the hardware */
#include "xil_types.h"		/* types used by xilinx */
#include "gic.h"

/* Defines */
// the modes the user can be in
#define CONFIGURE 0
#define PING 	  1
#define UPDATE 	  2
#define DONE 	  3

// which device to send to when uart_send is called
#define WIFI_DEV 0
#define TTY 	 1

#define NUM_WIFI_CALLBACKS 2*3

#define TO  	0
#define FRO 	1
#define C_TO 	2*CONFIGURE + TO
#define C_FRO   2*CONFIGURE + FRO
#define P_TO 	2*PING + TO
#define P_FRO	2*PING + FRO
#define U_TO 	2*UPDATE + TO
#define U_FRO	2*UPDATE + FRO

#define TRIG_LEVEL 1

// constants designating type sizes
#define PING_BYTE_COUNT   sizeof(ping_t)
#define UPDATE_BYTE_COUNT sizeof(update_response_t)

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

void uart_init(void (*wifi_callbacks[NUM_WIFI_CALLBACKS])(u8 buffer));

void uart_close(void);

void uart_send(u8 dev, void* addr, u32 size);
