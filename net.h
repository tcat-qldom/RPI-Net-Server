/*  Filename : net.h
 *
 *  Oberon Net server
 *  `SCC.Mod',`Net.Mod' recoded in cpp
 *
 * 22.05.2018 : [tcat] thomas.kral@email.cz
 *		Adapted for RISC-5 Oberon
 */

#ifndef net_h
#define net_h

#define HDR_SIZE 8
#define MAX_PAYLOAD 512
#define SUB_PACKET 32
#define MAX_PACKET (HDR_SIZE + MAX_PAYLOAD + SUB_PACKET-1) / SUB_PACKET * SUB_PACKET
#define PAK_SIZE 512
#define SEND_TRIES 5
#define WAIT 50 /* timeouts */
#define WAIT0 5
#define WAIT1 1000
#define T0 1000
#define T1 3000

typedef uint8_t payload[SUB_PACKET];

typedef struct {
	uint8_t valid, dadr, sadr, typ;
	int32_t len;
} header;

typedef struct {
	header hd;
	uint8_t dat[MAX_PACKET-HDR_SIZE];
} packet;

typedef enum {
	ACK = 0x10, NAK = 0x25, NPR = 0x26, /* acknowledgements */
    	NRQ = 0x34, NRS = 0x35, /* name request, response */
    	SND = 0x41, REC = 0x42, MSG = 0x44,
   	TRQ = 0x46, TIM = 0x47  /* time requests */
} message_type;

#endif
