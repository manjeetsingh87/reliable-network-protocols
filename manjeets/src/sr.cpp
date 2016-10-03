#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include<iostream>
#include <list>
#include <map>
#include <ctype.h>
#include <vector>
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
int base_A ;
int next_seqnum_A ;
int base_B;
int seqnum_B ;
int receiver_window;
float interrupt;

float ticker;

list<pkt> sender_pkt_buffer;
map<int, pkt> sent_to_B_map;
list<pkt> receiver_buffer;
list<pkt> receiver_acked_list;

struct interrupt_pkt {
	float interrupt;
	pkt packet;
};

vector<interrupt_pkt> interrupts_vctor;

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
		printf("Added packet with message %s to buffer. Message buffer size is %zu \n", packet.payload, sender_pkt_buffer.size());
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
		printf("Message received from layer 5 to be sent to B is : %s \n", A_packet.payload);
		printf("Computed checksum is: %d\n", A_packet.checksum);
		sent_to_B_map[next_seqnum_A] = A_packet;
		float clock_ticker = timeout_period+get_sim_time();
		struct interrupt_pkt temp;
		temp.interrupt = clock_ticker;
		temp.packet = A_packet;
		interrupts_vctor.push_back(temp);
		if(next_seqnum_A == base_A)
			starttimer(A, timeout_period);

		tolayer3(A, A_packet);
		next_seqnum_A++;
	} else {
		add_msg_to_buffer(A_packet);
	}
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet) {
	if(packet.checksum != compute_checksum(packet.seqnum, packet.acknum, packet.payload))
		printf("ACK packet was corrupted from B_input to A_input\n");
	else {
		sent_to_B_map.erase(packet.acknum);
		base_A = packet.acknum+1;
		if(base_A == next_seqnum_A)
			stoptimer(A);
		else {
			stoptimer(A);
			starttimer(A, timeout_period);
		}
		printf("Packets with sequence number %d acknowledged. Next sequence number generated for A is: %d\n", packet.acknum, next_seqnum_A);
		while((!sender_pkt_buffer.empty()) && (next_seqnum_A < base_A+windowsize)) {
			struct pkt buffered_pkt;
			buffered_pkt = sender_pkt_buffer.front();
			printf("got message from buffer to be sent: %d %s\n", buffered_pkt.seqnum, buffered_pkt.payload);
			sender_pkt_buffer.pop_front();
			tolayer3(A, buffered_pkt);
			sent_to_B_map[next_seqnum_A] = buffered_pkt;
			next_seqnum_A++;
			printf("current buffer size is %zu \n", sender_pkt_buffer.size());
		}
	}
	printf("-----------------------------------------------------------------------------\n");
}

bool is_interrupt_pkt_unacked(int seqnum) {
	map<int, pkt>::iterator itr;
	itr = sent_to_B_map.find(seqnum);
	if(itr != sent_to_B_map.end())
		return true;
	else
		return false;
}

/* called when A's timer goes off */
void A_timerinterrupt() {
	float interrupt_time = get_sim_time();
	for(int i=base_A; i<interrupts_vctor.size();i++) {
		interrupt_pkt resend_packet = interrupts_vctor.at(i);
		if(resend_packet.interrupt <= interrupt_time && (is_interrupt_pkt_unacked(resend_packet.packet.seqnum))) {
			resend_packet.interrupt = resend_packet.interrupt+timeout_period;
			printf("Re-sending packet with checksum %d and sequence number %d \n", resend_packet.packet.checksum, resend_packet.packet.seqnum);
			tolayer3(A, resend_packet.packet);
			if(resend_packet.packet.seqnum == base_A)
				stoptimer(A);
		}
	}
	float temp_interrupt = 0.0;
	for(int i=base_A; i<interrupts_vctor.size();i++) {
		interrupt_pkt resend_packet = interrupts_vctor.at(i);
		if(resend_packet.interrupt > interrupt_time && (is_interrupt_pkt_unacked(resend_packet.packet.seqnum))) {
			if(temp_interrupt == 0.0)
				temp_interrupt = resend_packet.interrupt;
			else if(temp_interrupt > resend_packet.interrupt)
				temp_interrupt = resend_packet.interrupt;
		}
	}
	if(temp_interrupt > 0.0)
		temp_interrupt = temp_interrupt-interrupt_time;
	else
		temp_interrupt = timeout_period;

	printf("next clock ticker set to :%f\n",temp_interrupt);

	/*started timer again with updated interrupt value*/
	starttimer(A, temp_interrupt);
}

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init() {
	windowsize = getwinsize();
	next_seqnum_A = 0;
	base_A = 0;
	interrupt = 0.0;
	ticker = 0.0;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */

/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet) {
	struct pkt B_packet;
	verify_checksum(&packet);
	if(!verify_checksum(&packet)) {
		printf("Packet corrupted at B\n");
		return;
	}
	printf("Received packet with sequence number %d and pay load %s at B\n",packet.seqnum, packet.payload);
	B_packet.acknum = packet.seqnum;
	B_packet.checksum = compute_checksum(packet.seqnum, packet.acknum, packet.payload);
	B_packet.seqnum = 0;
	for(int i=0; i<20; i++) {
		B_packet.payload[i] = packet.payload[i];
	}
	B_packet.payload[20] = '\0';
	if(packet.seqnum > base_B-receiver_window && packet.seqnum < base_B) {
		printf("Sending duplicate ACK to A for sequence number %d\n", packet.seqnum);
		tolayer3(B, B_packet);
	} else if((packet.seqnum >= base_B) && (packet.seqnum < base_B+receiver_window)) {
		if(packet.seqnum > base_B) {
			printf("Adding packet with sequence number %d and pay load %s to buffer at B\n", B_packet.acknum, B_packet.payload);
			receiver_buffer.push_back(B_packet);
		} else if(packet.seqnum == base_B) {
			list<pkt>::iterator it;
			for(it=receiver_buffer.begin(); it!=receiver_buffer.end(); ++it) {
				tolayer5(B, it->payload);
				printf("Sent buffered packet with pay load %s to layer 5 at B\n", it->payload);
			}
			tolayer5(B, B_packet.payload);
			printf("Sent packet with pay load %s to layer 5 at B\n", B_packet.payload);
			receiver_buffer.clear();
		}
		printf("Sending ACK to A \n");
		tolayer3(B, B_packet);
		base_B = packet.seqnum+1;
	}
}

/* the following routine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init() {
	seqnum_B = 0;
	receiver_window = getwinsize()/2;
	base_B = 0;
}
