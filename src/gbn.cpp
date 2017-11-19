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

struct windowItem{
  struct pkt p;
  int sent;
  int acked;
};

queue<struct pkt> A_pkt_buffer;   /* Queue to hold buffered send packets */

struct pkt pkts_sent[1000];       /* Packets Sent */
int A_last_ack_rcvd;              /* Last ACK Rcvd before timeout */

int N;                            /* Max Window Size */

int base;                         /* Base of window */
int nextseqnum;                   /* Next Seq Number */

//float MAX_TIMEOUT;                /* Max Timeout Value */
//float MIN_TIMEOUT;                /* Minimum Timeout Value */
static float TIMEOUT = 30;          /* timeout before timer interrupt is called */   

int expectedseqnum;               /* Expected seqnum of next packet */
int last_delivered_seqnum;        /* Store last delivered seqnumber */

void printpacketinfo(struct pkt pkt){;
  printf("Packet Information:\n");
  printf("seqnum = %d\n",pkt.seqnum);
  printf("acknum = %d\n",pkt.acknum);
  printf("checksum = %d\n",pkt.checksum);
  printf("Payload = %s\n",pkt.payload);
  printf("Payload Length = %lu\n\n",strlen(pkt.payload));
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
  TIMEOUT = TIMEOUT++;
  if(TIMEOUT >  MAX_TIMEOUT){
    TIMEOUT = MAX_TIMEOUT;
  }
}
void decTimeout(){
  TIMEOUT = TIMEOUT--;
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
  
  /* check if next seqnum is outside of window OR there are buffered packets */
  if(nextseqnum < base + N && A_pkt_buffer.size() == 0){
    /* Send Packet */
    pkt.seqnum = nextseqnum;
    pkt.checksum = compute_checksum(pkt);

    /* Add packet to packets sent */
    pkts_sent[nextseqnum] = pkt;

    tolayer3(0,pkt);
    

    /* Move Window Forward by setting base to seq num */
    if(base == nextseqnum){
      starttimer(0,TIMEOUT);
    }

    /* increment next seq num */
    nextseqnum++;
  }else{
    /* Buffer Packet to be sent later */
    A_pkt_buffer.push(pkt);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt packet)
{

  /* Check if ACK is corrupted */
  int checksum = compute_checksum(packet);
  
  if(checksum != packet.checksum){
    /* ACK Corrupted. Let Timeout */
    //decTimeout();
  }else{
    /* Update Packet in Packet Window as ACKED */
    //incTimeout();
    
    /* Increment Base */
    base = packet.acknum + 1;
    A_last_ack_rcvd = packet.acknum;


    if(base == nextseqnum){
      stoptimer(0);
      /* B has received entire window. Stop Timer*/

      /* Send Next Packet in Buffer */
      if(A_pkt_buffer.size() > 0){  
        int i = 1;
        while(i < N && A_pkt_buffer.size() > 0){

          /* Pop Front of buffer */
          struct pkt pkt = A_pkt_buffer.front();
          A_pkt_buffer.pop();

          /* Make sure sequence number is not outside the window */
          if(nextseqnum < base + N){

          /* Send Packet */
          pkt.seqnum = nextseqnum;
          pkt.checksum = compute_checksum(pkt);
      
          /* Add Packet to pkts sent */
          pkts_sent[nextseqnum] = pkt;
      
          /* Send packet to B */
          tolayer3(0,pkt);
      
          /* Move Window Forward by setting base to seq num */
          if(base == nextseqnum){
            starttimer(0,TIMEOUT);
          }

          /* Increment Next Seq Num */
          nextseqnum++;
        }
        i++;
      }
    }
  }else{
    starttimer(0,TIMEOUT);
    }
  }
}

/* called when A's timer goes off */
void A_timerinterrupt()
{
  base = A_last_ack_rcvd;
  starttimer(0,TIMEOUT);
  for(int i = base; i < nextseqnum; i++){
    struct pkt p = pkts_sent[i];
    tolayer3(0,p);
  }
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  base = 0;                 /* Start base at 1 */
  nextseqnum = 0;           /* Start nextseqnum at 1 */
  N = getwinsize();         /* Window Size */

  /* Set Initial Timeout */
  //MAX_TIMEOUT = 20 + N;
  //TIMEOUT = MAX_TIMEOUT;
  //MIN_TIMEOUT = 10;
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt packet)
{

  int checksum = 0;
  checksum = compute_checksum(packet);

  if(checksum == packet.checksum && packet.seqnum == expectedseqnum){
    /* Deliver Packet to layer 5 */
    tolayer5(1,packet.payload);

    /* Send ACK Packet to A*/
    struct pkt p;
    p.acknum = expectedseqnum;
    p.checksum = compute_checksum(p);
    last_delivered_seqnum = expectedseqnum;
    tolayer3(1,p);

    /* Incremement expected seq num by 1 */
    expectedseqnum++;
  }else if(checksum != packet.checksum){
    /* Do Nothing. Let Sender Timeout and resend window */
  }else{
    /* Send ACK for last packet */
    struct pkt p;
    p.acknum = last_delivered_seqnum; 
    p.checksum = compute_checksum(p);
    tolayer3(1,p);
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  expectedseqnum = 0;
}