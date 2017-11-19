#include "../include/simulator.h"
#include <queue>
#include <stdio.h>
#include <string.h>

using namespace std;

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

queue<struct pkt> pkt_buffer;   /* Queue to hold buffered Packets */
int nextseq;                    /* Next Sequence Number to use */

//float MAX_TIMEOUT;          /* Max Timeout Value */
//float MIN_TIMEOUT;          /* Minimum Timeout Value */
static float TIMEOUT = 20;         /* timeout before timer interrupt is called */  

struct pkt last_sent_pkt;       /* Last sent packet */
int last_seq_num;                /* Last Sequence number delivered */
bool packet_unacked;             /* boolean to tell if there is a packet unacked */
int pkts_sent;                  /* Keep track of pkts sent */

void printpacketinfo(struct pkt pkt){;
  printf("Packet Information:\n");
  printf("seqnum = %d\n",pkt.seqnum);
  printf("acknum = %d\n",pkt.acknum);
  printf("checksum = %d\n",pkt.checksum);
}

int compute_checksum(struct pkt p){
  int checksum = 0;
  int payload_sum = 0;

  for(int i = 0; i < sizeof(p.payload); i ++){
    payload_sum = payload_sum + p.payload[i];
  }
  checksum = p.seqnum + p.acknum + payload_sum;
  return checksum;
}

/*
void incTimeout(){
  TIMEOUT ++;
  if(TIMEOUT >  MAX_TIMEOUT){
    TIMEOUT = MAX_TIMEOUT;
  }
}
void decTimeout(){
  TIMEOUT --;
  if(TIMEOUT < MIN_TIMEOUT){
    TIMEOUT = MIN_TIMEOUT;
  }
}
*/


/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{
  /* Build new packet */
  pkt pkt;

  /* Copy Payload data */
  memcpy(pkt.payload,message.data,sizeof(message.data));

  if(pkt_buffer.size() > 0 || packet_unacked == true){
    /* There is an unacked packet OR there are packets in the buffer - Buffer Packet */
    pkt_buffer.push(pkt);
  }else{
    /* No Packets in Buffer AND No Packets on wire - Send Packet to B */

    /* Fill out the rest of the packet data*/
    pkt.seqnum = nextseq;
    pkt.acknum = nextseq;
    pkt.checksum = compute_checksum(pkt);
    pkts_sent++;

    /* Send to layer 3*/
    tolayer3(0,pkt);
    starttimer(0,TIMEOUT);
    last_sent_pkt = pkt;
    packet_unacked = true;


  }
  
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

  /* Check if ACK is corrupted */
  int checksum = compute_checksum(packet);
  
  if(checksum != packet.checksum || packet.acknum != last_sent_pkt.seqnum){
    /*  ACK Packet is corrupt or ACK is not for correct packet. 
        Do not stop timer. 
        Allow timerinterrupt() to happen */

    /* Reduce timeout by 1 */
    //decTimeout();
  
  }else if(checksum == packet.checksum && packet.acknum == last_sent_pkt.seqnum){
    /*  Packet is not corrupt AND acknum is correct
        Process ACK , send another packet from buffer */
    //incTimeout();
    
    stoptimer(0);
    packet_unacked = false;

    /* Set Next seqnum */
    if(packet.acknum == 1){
      nextseq = 0;
    }else{
      nextseq = 1;
    }

    /* Send Next Packet in Buffer to B*/
    if(pkt_buffer.size() > 0){

      struct pkt p = pkt_buffer.front();
      pkt_buffer.pop();

      /* Add values to packet struct */
      p.seqnum = nextseq;
      p.acknum = nextseq;
      p.checksum = compute_checksum(p);

      /* Incremement pkts_sent */
      pkts_sent ++;

      /* Send Packet to B */
      starttimer(0,TIMEOUT);
      tolayer3(0,p);
      last_sent_pkt = p;
      packet_unacked = true;
      
    }
  }
}
/* called when A's timer goes off */
void A_timerinterrupt()
{
  /* Send last sent packet again and start the timer */
  tolayer3(0,last_sent_pkt);
  starttimer(0,TIMEOUT);


}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  //printf("Starting Program\n");
  nextseq = 0;              /* start sequence number at 0 */
  packet_unacked = false;   /* start unacked packet at false */
  pkts_sent = 0;

  //MAX_TIMEOUT = 20;
  //MIN_TIMEOUT = 4;
  //TIMEOUT = MAX_TIMEOUT;     /* Start timeout at Max value */
  //printf("Done");
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{
  /* Packet is valid - Send ACK to A and send to layer5 */

  int checksum = 0;
  checksum = compute_checksum(packet);

  if(checksum == packet.checksum && packet.seqnum != last_seq_num){
    /* Pass data to Layer 5 */
    last_seq_num = packet.seqnum;
    tolayer5(1,packet.payload);

    /* Packet is valid. Send ack to A */
    pkt ackpkt;
    ackpkt.seqnum = packet.seqnum;
    ackpkt.acknum = packet.seqnum;
    ackpkt.checksum = compute_checksum(ackpkt);
    tolayer3(1,ackpkt);

  }else if(packet.seqnum == last_seq_num){
    pkt ackpkt;
    ackpkt.seqnum = packet.seqnum;
    ackpkt.acknum = packet.seqnum;
    ackpkt.checksum = compute_checksum(ackpkt);
    tolayer3(1,ackpkt);

  }else{
      /* Do Nothing Packet is Corrupt. This will force A to send based on timeout. */
  }


}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  last_seq_num = 1;
}
