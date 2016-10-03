#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include<iostream>
#include <list>
#include <map>
#include "../include/simulator.h"

using namespace std;

#define default_timeout 15.0
#define A 0
#define B 1
/* ******************************************************************
 ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.1  J.F.Kurose

 This code should be used for PA2, unidirectional data transfer
 protocols (from A to B). Network properties:
 - one way network delay averages five time units (longer if there
 are other messages in the channel for GBN), but can be larger
 - packets can be corrupted (either the header or the data portion)
 or lost, according to user-defined probabilities
 - packets will be delivered in the order in which they were sent
 (although some can be lost).
 **********************************************************************/

/********* STUDENTS WRITE THE NEXT SEVEN ROUTINES *********/

int next_seqnum_A;
bool is_last_pkt_ack;

int seqnum_B;
map<int, pkt> sent_to_B_map;

/*
 * computes the checksum to be verified between sender and receiver end.
 * Checksum = 20msgbits * 8 bits + acknum + seqnum
 */
int compute_checksum(int seqnum, int acknum, char msg[20]) {
	int checksum = 0;
	if(msg == NULL)
		return checksum;

	int msgbits = 0;
	for(int i=0; i < 20 ; i++) {
		msgbits += msg[i];
	}
	checksum = msgbits+seqnum+acknum;
	return ~checksum;
}

bool verify_checksum(pkt *packet) {
	int correct_pkt = ~0;
	int msgbits = 0;
	for(int i=0; i < 20 ; i++) {
		msgbits += packet->payload[i];
	}
	int received_checksum = packet->seqnum+packet->acknum+msgbits;
	received_checksum = received_checksum+packet->checksum;
	if(received_checksum == correct_pkt)
		return true;
	else
		return false;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
	struct pkt A_packet;

	if(!sent_to_B_map.empty())
		printf("Last packet ACK not yet received. Dropping this packet \n");
	else {
		for(int i=0; i<strlen(message.data); i++) {
			if(isalpha(message.data[i]))
				A_packet.payload[i] = message.data[i];
		}
		A_packet.payload[20] = '\0';
		printf("Message received from layer 5 to be sent to B is : %s \n", A_packet.payload);
		A_packet.seqnum = next_seqnum_A;
		A_packet.acknum = 0;
		A_packet.checksum = compute_checksum(A_packet.seqnum, A_packet.acknum, A_packet.payload);
		printf("Computed checksum is: %d\n", A_packet.checksum);
		tolayer3(A, A_packet);
		sent_to_B_map[next_seqnum_A] = A_packet;
		starttimer(A, default_timeout);
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	if(packet.checksum != compute_checksum(packet.seqnum, packet.acknum, packet.payload))
		printf("ACK packet was corrupted \n");
	else if(packet.acknum != next_seqnum_A)
		printf("Unexpected ACK received \n");
	else {
		sent_to_B_map.erase(next_seqnum_A);
		next_seqnum_A = next_seqnum_A+1;
		is_last_pkt_ack = true;
		printf("Last packet acknowledged. Next sequence number generated for A is: %d\n", next_seqnum_A);
		stoptimer(A);
	}
	printf("-----------------------------------------------------------------------------\n");
}

/* called when A's timer goes off */
void A_timerinterrupt() {
	printf("Timer interrupt occurred for sequence number %d\n", next_seqnum_A);
	map<int, pkt>::iterator itr;
	itr = sent_to_B_map.find(next_seqnum_A);
	if(itr != sent_to_B_map.end()) {
		struct pkt retransmit_pkt;
		retransmit_pkt = itr->second;
		printf("Re-sending packet with checksum %d and sequence number %d \n", retransmit_pkt.checksum, retransmit_pkt.seqnum);
		tolayer3(A, retransmit_pkt);
		starttimer(A, default_timeout);
	}
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	next_seqnum_A = 0;
	is_last_pkt_ack = false;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	struct pkt B_packet;

	verify_checksum(&packet);
	/*
	 * to verify the checksum received from A and the computed checksum at B. if they don't match, return and print packet was corrupted
	 */
	if(!verify_checksum(&packet))
		printf("Packet corrupted \n");
	else {
		B_packet.seqnum = 0;
		B_packet.acknum = packet.seqnum;
		B_packet.checksum = compute_checksum(packet.seqnum, packet.acknum, packet.payload);
		for(int i=0; i<strlen(packet.payload); i++) {
			if(isalpha(packet.payload[i]))
				B_packet.payload[i] = packet.payload[i];
		}
		B_packet.payload[20] = '\0';
		if(packet.seqnum < seqnum_B)
			printf("Duplicate packet received at B \n");
		else if(packet.seqnum == seqnum_B){
			seqnum_B = seqnum_B+1;
			printf("Sending packet to later 5 at B \n");
			tolayer5(B, B_packet.payload);
		}
		printf("Sending ACK to A \n");
		tolayer3(B, B_packet);
	}
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
	seqnum_B = 0;
}
