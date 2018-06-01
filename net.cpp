/*  Filename : net.cpp
 *
 *  Oberon Net server
 *  `SCC.Mod',`Net.Mod' recoded in cpp
 *
 * 22.05.2018 : [tcat] thomas.kral@email.cz
 *		Adapted for RISC-5 Oberon
 */

#include <cstdlib>
#include <iostream>
#include <time.h>
#include <sys/stat.h>
#include <RF24/RF24.h>
#include "net.h"

using namespace std;

// Default pipe addresses after radio reset
const uint64_t pipes[6] = { 0xe7e7e7e7e7LL, 0xc2c2c2c2c2LL,
				 0xc3, 0xc4, 0xc5, 0xc6 
			  };

// CE Pin, CSN Pin, SPI Speed

// Setup for GPIO 22 CE and CE0 CSN with SPI Speed @ 4Mhz
RF24 radio(RPI_V2_GPIO_P1_22, BCM2835_SPI_CS0, BCM2835_SPI_SPEED_4MHZ); 

// Setup for GPIO_25 and /dev/spidev0.0
// RF24 radio(25,0);

// Globals
packet rx;				/* receive packet */
header head0, head1;	/* packet headers */
bool filter, protect;
uint8_t Adr, dmy[8];
int32_t rcvdc;			/* received bytes counter */
char partner[8];


void setup()
{
	// Default radio setting
	radio.begin();
	radio.setRetries(10,15);
	radio.setAutoAck(true);
	radio.setPALevel(1);
	radio.setChannel(5);
	radio.maskIRQ(1,1,1);
	/*radio.txDelay = 15;*/

	// Enable pipe-0 for reading and writing
	/*radio.openWritingPipe(pipes[0]);*/
	radio.openReadingPipe(0,pipes[0]);

	// Start listening
	// Dump radio configuration for debugging
	radio.startListening();
	radio.printDetails();
	
	printf("Output below [Ctrl-C to exit]: \n");
	delay(1);
}

char itoa( uint8_t b )
{
	char ch[2]; sprintf(ch, "%d", b);
	return ch[0];
}

void setpartner( char *name )
{
	head0.dadr = head1.sadr; strcpy(partner, name);
}

bool rcvd( uint32_t time )
{
	time = time + millis();
	bool timeout = false;
	while ( ! radio.available() && ! timeout )
		if (millis() >=time ) timeout = true;
	return !timeout;
}

void subsnd( payload *pl, bool *timeout )
{
	uint32_t stry = SEND_TRIES;

	/*header *head;
	head = (header *) pl;
	printf ("%d %d %d %d ", head->typ, head->len, head->sadr, head->dadr);*/

	bool done = radio.write(pl, SUB_PACKET, stry);

	*timeout = ! done;
	//printf ("%d\n", stry);
}

void resetrcv()
{
	memset(&rx, 0, HDR_SIZE); rcvdc = 0;
}

void receive( uint8_t *x )
{	/* packet already rcvd */
	if ( rcvdc < rx.hd.len ) *x = rx.dat[rcvdc++]; 
	else *x = 0;
}

void skip( uint32_t m )
{
	uint8_t dmy;
	while ( m-- != 0 ) receive(&dmy);
}

void sendpacket( header *head, uint8_t *dat )
{
	int32_t len, i, off;
	bool timeout;
	payload pl;

	radio.stopListening();
	if ( Adr == 0 ) Adr = millis() & 0xff;
	head->sadr = Adr; head->valid = true;
 	memcpy(&pl, head, HDR_SIZE);

	/*header *hd;
	hd = (header *) &pl;
	printf ("%d %d %d %d\n", hd->typ, hd->len, hd->sadr, hd->dadr);*/

	i = HDR_SIZE; off = 0; len = head->len;
	while ( len && ( i < SUB_PACKET ) ) {
		pl[i++] = dat[off++]; --len;
	}
	while ( i < SUB_PACKET ) pl[i++] = 0;
	subsnd(&pl, &timeout);
	while (! timeout && len ) /* send the rest */
	{
		i = 0;
		while ( len && ( i < SUB_PACKET ) ) {
			pl[i++] = dat[off++]; --len;
		}
		while ( i < SUB_PACKET ) pl[i++] = 0;
		subsnd(&pl, &timeout);
	}
	radio.startListening();
}

void send( uint8_t t, uint32_t l, uint8_t *data )
{
	head0.typ = t; head0.len = l; sendpacket(&head0, data);	
}

void receivehead( header *head ) /* actually, recv whole packet */
{
	int32_t n;
	payload *adr;

	head->valid = false;
	if ( rcvd(0) ) {
		resetrcv(); adr = (payload *) &rx;	
		radio.read(adr, SUB_PACKET);
		n = (rx.hd.len + HDR_SIZE - 1) / SUB_PACKET;
		if ( (rx.hd.len <= MAX_PAYLOAD) &&
			( (rx.hd.dadr == 0xff) || ! filter || (Adr == 0) ||
			  (rx.hd.dadr == Adr) )
			) {
			while ( n && rcvd(WAIT1) ) {
				radio.read(++adr, SUB_PACKET);
				--n;
			}
			rx.hd.valid = ( n == 0 );
		} else {
			printf(" discarded\n");
			while ( rcvd(WAIT1) ) /* discard packet */
				radio.read(adr, SUB_PACKET);
			resetrcv(); 
		}
		*head = rx.hd;
	}
}

void receivehead( const uint32_t timeout )
{
	uint32_t time = millis() + timeout;
	do {
		receivehead(&head1);
		/*printf ("%d %d %d %d\n", \
			head1.typ, head1.len, head1.sadr, head1.dadr);*/
		if ( head1.valid && ( head1.sadr != head0.dadr ) ) {
			printf(" skipped\n");
			skip(head1.len); head1.valid = false;
		}
		if ( ! head1.valid && ( millis() >= time ) ) head1.typ = 0xff;
	} while ( ! head1.valid && ( head1.typ != 0xff ) );
}

void uxobtime( int32_t *obtime )
{
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	/* printf("%s", asctime(tm));
	printf("%d %d %d %d %d %d\n", tm->tm_year-100, tm->tm_mon+1, \
		tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec ); */
	*obtime = (((((tm->tm_year-100)*16 + tm->tm_mon+1)*32 \
		+ tm->tm_mday)*32 + tm->tm_hour)*64 \
		+ tm->tm_min)*64 + tm->tm_sec; 
}

void appends( const char s[], uint8_t d[], int32_t *k )
{
	int32_t i, j;
	char ch;
	i = 0; j = *k;
	do { ch = s[i++]; d[j++] = ch; } while ( ch != 0 );
	*k = j;
}

void appendw( int32_t s, uint8_t d[], int32_t n, int32_t *k )
{
	int32_t i = *k;
	do { d[i++] = s & 0xff; s = s >> 8; } while( --n );
	*k = i;
}

void picks( char s[] )
{
	int32_t i;
	uint8_t x;
	i = 0;
	do { receive(&x); s[i++] = x; } while ( x != 0 );
}

void pickq( int32_t *w )
{
	uint8_t x0, x1, x2, x3;
	receive(&x0); receive(&x1); receive(&x2); receive(&x3); 
	*w = x0 | x1 << 8 | x2 << 16 | x3 << 24; 
}

void senddata( FILE *f )
{
	int32_t k, seqno, len;
	uint8_t buf[PAK_SIZE];

	len = 0; seqno = 0;
	do {
		k = fread(&buf, 1, PAK_SIZE, f);
		do {
			send(seqno, k, buf); receivehead(T1);
			printf ("%d %d %d %d %d\n", \
				head1.typ, head1.len, head1.sadr, head1.dadr, k);
		} while ( head1.typ == (seqno + ACK) );
		seqno = (seqno + 1) % 8; len += k;
		if ( head1.typ != (seqno + ACK) ) { printf(" failed"); k = 0; }
	} while ( k == PAK_SIZE );
	printf(" %d\n", len);
}

void receivedata( FILE *f, bool *done )
{
	int32_t k, retry, seqno, len;
	
	seqno = len = 0; retry = 2;
	do {
		if ( head1.typ == seqno ) {
			seqno = (seqno + 1) % 8; len += head1.len; retry = 2;
			send(seqno + ACK, 0 , dmy); k = 0;
			k = fwrite(&rx.dat, 1, head1.len, f);
	      printf ("%d %d %d %d %d\n", \
				head1.typ, head1.len, head1.sadr, head1.dadr, k);
			if ( k < PAK_SIZE ) *done = true;
		} else {
			if ( --retry == 0 ) { printf(" failed"); *done = false; k = 0; }
			send(seqno + ACK, 0 , dmy);
		}
		receivehead(T0);
	} while ( k == PAK_SIZE );
	printf(" %d\n", len);
}

void reply( int32_t msg )
{
	switch ( msg ) {
		//case 0: head1.typ = 0; break; /* reset state machine */
		case 1: printf(" no link"); break;
		case 2: printf(" no permission"); break;
		case 3: printf(" not done"); break;
		case 4: printf(" not found"); break;
		case 5: printf(" no response"); break;
		case 6: printf(" time set"); break;
	}
	printf("\n");
}

int main( int argc, char** argv ) 
{
	int32_t i, pw, obtime, time;
	char id[8], filename[32], newf[32];
	uint8_t x, idb[12];
	bool done;
	FILE *f;

	setup();
	filter = true;
	protect = false;
	time = millis();

	while(true)
	{
		receivehead(&head1);
		if ( head1.valid ) { 
			cout << "valid header \n";
			printf ("%d %d %d %d\n", head1.typ, head1.len, head1.sadr, head1.dadr);
			switch ( head1.typ ) {
				case SND:
					picks(id); pickq(&pw); picks(filename);
					printf("%s %s", id, filename);
					f = fopen(filename, "r");
					if ( f != NULL ) {
						printf(" sending\n"); setpartner(id);
						senddata(f); send(ACK, 0, dmy); fclose(f);
					} else { send(NAK, 0, dmy); printf("~\n"); }
					reply(0);
					break;
				case REC:
					picks(id); pickq(&pw); picks(filename);
					printf("%s %s", id, filename);
					strcpy(newf, filename);
					f = fopen(strcat(filename, "~"), "w");
					if ( f != NULL ) {
						printf(" receiving\n"); setpartner(id);
						send(ACK, 0, dmy); receivehead(T0); receivedata(f, &done);
						fclose(f); chmod(filename, 0664);
						if ( done ) rename(filename, newf);
					} else { send(NAK, 0, dmy); printf("~\n"); }
					reply(0);
					break;
				case NRQ:
					cout << "name request\n";
					i = 0; do { 
						receive(&x); id[i++] = x;
						if (i == 7) { id[7] = 0; x = 0; }
					} while ( x != 0 );
					while (i++ < head1.len) receive(&x);
					printf ( "%s\n", id );
					if ( strcmp(id, "RPI") == 0 ) {
						setpartner(id); send(NRS, 0, dmy);
						printf("%s set\n", partner);
					}
					break;
				case MSG:
					cout << "message\n";
					i = 0; while ( i++ < head1.len ) {
						receive(&x); printf("%c", x);
					}
					cout << "\n";
					send(ACK, 0, dmy); reply(0);
					break;
				case TRQ:
					cout << "time request\n";
					uxobtime(&obtime);
					i = 0; appendw(obtime, idb, 4, &i);
					send(TIM, 4, idb);
					break;
				default:
					cout << "skipped\n";
					skip (head1.len);
			}
		} /* if ( head1.valid ) */

		else if ( millis() >= time + 16000 ) {
			head0.dadr = 0xff; uxobtime(&obtime);
			i = 0; appendw(obtime, idb, 4, &i);
			appends("RPI", idb, &i);
			appendw(obtime, idb, 4, &i);
			send(TIM, i, idb); time = millis();
			cout << "TIM broadcast\n";
			//radio.flush_tx();
		} else
			delay(1);
	}
	return 0;
}

