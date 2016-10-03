#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include<iostream>
#include <list>
#include <map>
#include <ctype.h>
#include "../include/simulator.h"

using namespace std;

#define timeout_period 15.0
#define A 0
#define B 1
#define MAX_BUFFER_SIZE 1000
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
int windowsize ;
int next_seqnum_A ;
int seqnum_B ;
int base_A ;
list<pkt> sender_pkt_buffer;
map<int, pkt> sent_to_B_map;

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

int msg_present_in_buffer (struct pkt message) {
	int is_present = 1;
	if(!sender_pkt_buffer.empty()) {
		list<pkt>::iterator it;
		for(it=sender_pkt_buffer.begin(); it != sender_pkt_buffer.end(); ++it) {
			if(it->seqnum == message.seqnum) {
				is_present = 0;
				break;
			}
		}
	}
	return is_present;
}

void add_msg_to_buffer(struct pkt packet) {
	if(sender_pkt_buffer.size() >= MAX_BUFFER_SIZE) {
		printf("Message buffer saturated, give up and exit GBN\n");
		exit(1);
	} else if(msg_present_in_buffer(packet) == 1) {
		sender_pkt_buffer.push_back(packet);
		printf("Added packet with message %s to buffer.\n", packet.payload);
	}
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message) {
	struct pkt A_packet;

	A_packet.seqnum = next_seqnum_A;
	A_packet.acknum = 0;
	for(int i=0; i<20; i++) {
		A_packet.payload[i] = message.data[i];
	}
	A_packet.payload[20] = '\0';
	A_packet.checksum = compute_checksum(A_packet.seqnum, A_packet.acknum, A_packet.payload);

	if(next_seqnum_A < base_A+windowsize) {
		printf("Message received from layer 5 with checksum %d to be sent to B is : %s \n", A_packet.checksum, A_packet.payload);
		tolayer3(A, A_packet);
		sent_to_B_map[next_seqnum_A] = A_packet;
		if(next_seqnum_A == base_A)
			starttimer(A, timeout_period);

		next_seqnum_A++;
	} else
		add_msg_to_buffer(A_packet);
}

/* remove all cumulative acknowledged packets*/
void remove_ackedpckts(int acknum) {
	map<int, pkt>::iterator itr;
	for(itr=sent_to_B_map.begin(); itr!=sent_to_B_map.end(); ++itr) {
		if(itr->first <= acknum) {
			printf("Removing acknowledged packet : %d\n", itr->first);
			sent_to_B_map.erase(itr);
		}
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	if(packet.checksum != compute_checksum(packet.seqnum, packet.acknum, packet.payload))
		printf("ACK packet was corrupted from B_input to A_input\n");
	else if(packet.acknum < base_A)
		printf("Unexpected ACK received at A_input\n");
	else {
		printf("Last packet acknowledged. Next sequence number generated for A is: %d\n", next_seqnum_A);
		remove_ackedpckts(packet.acknum);
		base_A = packet.acknum+1;
		if(base_A == next_seqnum_A)
			stoptimer(A);
		else {
			stoptimer(A);
			starttimer(A, timeout_period);
		}

		/* send all the unacknowledged packets again which fall within the sender window size */
		while((!sender_pkt_buffer.empty()) && (next_seqnum_A < base_A+windowsize)) {
			struct pkt buffered_pkt;
			buffered_pkt = sender_pkt_buffer.front();
			printf("got message from buffer to be sent: %d %s\n", buffered_pkt.seqnum, buffered_pkt.payload);
			sender_pkt_buffer.pop_front();

			tolayer3(A, buffered_pkt);
			sent_to_B_map[next_seqnum_A] = buffered_pkt;

			next_seqnum_A++;
		}
	}
	printf("-----------------------------------------------------------------------------\n");
}

/* called when A's timer goes off */
void A_timerinterrupt() {
	starttimer(A, timeout_period);
	map<int, pkt>::iterator itr;
	for(itr=sent_to_B_map.begin(); itr!=sent_to_B_map.end(); ++itr) {
		struct pkt retransmit_pkt;
		retransmit_pkt = itr->second;
		printf("Re-sending packet with checksum %d and sequence number %d \n", retransmit_pkt.checksum, retransmit_pkt.seqnum);
		tolayer3(A, retransmit_pkt);
	}
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	windowsize = getwinsize();
	next_seqnum_A = 0;
	base_A = 0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	struct pkt B_packet;

	verify_checksum(&packet);

	if(!verify_checksum(&packet)) {
		printf("Packet corrupted \n");
		B_packet.acknum = seqnum_B-1;
		printf("Sending ACK for out of order packet to A\n");
		tolayer3(B, B_packet);
		return;
	}
	B_packet.checksum = compute_checksum(packet.seqnum, packet.acknum, packet.payload);
	B_packet.seqnum = 0;
	for(int i=0; i<20; i++) {
		B_packet.payload[i] = packet.payload[i];
	}
	B_packet.payload[20] = '\0';
	if(packet.seqnum == seqnum_B){
		printf("last acknowledgeed packet is: %d\n", seqnum_B);
		B_packet.acknum = packet.seqnum;
		tolayer5(B, B_packet.payload);
		printf("Sent packet to layer 5 at B\n");
		seqnum_B = packet.seqnum+1;
		printf("Sending ACK for packet %d to A \n", packet.seqnum);
		tolayer3(B, B_packet);
	} else {
		B_packet.acknum = seqnum_B-1;
		printf("Sending ACK for out of order packet to A\n");
		tolayer3(B, B_packet);
	}
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
	seqnum_B = 0;
}
