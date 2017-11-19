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

struct windowItem{
  float timesent;
  struct pkt p;
  int delivered;
};

/* A Variables */
queue<struct pkt> A_pkt_buffer;         /* Queue to hold buffered send packets */
struct windowItem pkts_sent[1000];      /* Packets Sent with Sent Time */

int N;                            /* Window Size */
int base;                         /* Base of window */
int nextseqnum;                   /* Next Seq Number */

static float TIMEOUT = 20;        /* timeout before timer interrupt is called */     

pair<float,struct windowItem> timer_start_pkt;  /* Pair to hold the start time and pkt that started it */

int pkts_unacked;                 /* Keep track of unacked pkts */

/* B Variables */
int rcv_base;                     /* Base for receiver */
struct windowItem pkts_recvd[1000];      /* Buffered packets for B, recver */


void printpacketinfo(int AorB,struct windowItem w){
  if( AorB == 0){
    printf("    Packet Information:\n");
    printf("    Seq Num = %d\n",w.p.seqnum);
    printf("    Ack Num = %d\n",w.p.acknum);
    printf("    Time Sent   = %f\n",w.timesent);
  }else if(AorB == 1){
    printf("    Packet Information:\n");
    printf("    Seq Num =  %d\n",w.p.seqnum);
    printf("    Seq Num =  %d\n",w.p.acknum);
    printf("    delivered   = %f\n",w.timesent);
  }
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
void updateTimerStartTime(float _time,struct windowItem w){
  timer_start_pkt.first = _time;
  timer_start_pkt.second = w;
}
float getTimerStartTime(){
  return timer_start_pkt.first;
}
struct windowItem getTimerStartWindowItem(){
  return timer_start_pkt.second;
}

/* called from layer 5, passed the data to be sent to other side */
void A_output(struct msg message)
{

  /* Build new packet */
  pkt pkt;

  /* Copy Payload data */
  memcpy(pkt.payload,message.data,sizeof(message.data));
  
  /* check if next seqnum is outside of window OR there are buffered packets */
  if(nextseqnum < base + N && A_pkt_buffer.size() == 0){

    /* Set Packet Values */
    pkt.seqnum = nextseqnum;
    pkt.acknum = -1;
    pkt.checksum = compute_checksum(pkt);
    

    /* Add packet to packets sent */
    windowItem w;
    w.p = pkt;
    w.timesent = get_sim_time();
    pkts_sent[nextseqnum] = w;


    /* Start timer if first packet sent in window */
    int val = nextseqnum - base;
    if(val == 0){     
      starttimer(0,TIMEOUT);
      updateTimerStartTime(get_sim_time(),w);
    }

    
    /* Send to layer 3 */
    pkts_unacked++;
    tolayer3(0,pkt);

    /* Increment next seq num */
    nextseqnum++;
  }else{
    /* Buffer Packet to be sent later */
    A_pkt_buffer.push(pkt);
  }
}

/* called from layer 3, when a packet arrives for layer 4 */
void A_input(struct pkt ack_packet)
{

  /* Check if ACK is corrupted */
  int checksum = compute_checksum(ack_packet);
  

  if(checksum != ack_packet.checksum){
    /* ACK Corrupted. Let Timeout */

  }else if(ack_packet.acknum >= base && ack_packet.acknum <= base + N){
    /* Print Ack Packet information */
    struct windowItem temp;
    temp.p = ack_packet;
    temp.timesent = -1;

    /* Stop Timer if Packet is the packet that started the timer */
    struct windowItem timer_start_packet = getTimerStartWindowItem();
    if(ack_packet.acknum == timer_start_packet.p.seqnum){
      stoptimer(0);
    }

    /* Set Packet in pkts_sent as acked */
    pkts_sent[ack_packet.acknum].p.acknum = ack_packet.acknum;
    pkts_unacked --;

    /* Increment Base */
    if(ack_packet.acknum == base){

      base = ack_packet.acknum + 1;
      for(int i = base; i <= nextseqnum; i++){
        if(pkts_sent[i].p.seqnum == 0){
          /* Found Next Unacked Packet */
          base = i;
          break;
        }
      }
    }

    /* Send a buffered packet if able to */
    if(nextseqnum < base + N){

      /* Send Next Packet in Buffer */
      if(A_pkt_buffer.size() > 0){  
        int i = 1;

        while(i < N && A_pkt_buffer.size() > 0){

          /* Make sure sequence number is not outside the window */
          if(nextseqnum < base + N){

          /* Pop Front of buffer */
          struct pkt pkt = A_pkt_buffer.front();
          A_pkt_buffer.pop();

          /* Set Packet Values */
          pkt.seqnum = nextseqnum;
          pkt.acknum = -1;
          pkt.checksum = compute_checksum(pkt);
          
          /* Add packet to packets sent */
          windowItem w;
          w.p = pkt;
          w.timesent = get_sim_time();
          pkts_sent[nextseqnum] = w;

          /* Start timer if first packet sent in window */
          if(pkt.seqnum == base){
            starttimer(0,TIMEOUT);
            updateTimerStartTime(get_sim_time(),w);

            /* Reset Number of Packets sent last window */

          }

          /* Send to layer 3 */
          tolayer3(0,pkt);
          pkts_unacked++;

          /* Increment next seq num */

          nextseqnum++;
          }
        i++;
        }
      } 
    }
  }else{
    /* Ack falls outside of the packet window */
  }
}

/*

Here is where simulated logical timers will occur.

A timer is started when the first packet in the window is sent. Basically, when
base = nextseqnum. Since every subsequent packet after that will be sent at a time, 
after that send time. There timeout will basically be TIMEOUT + (START_TIME - INITIAL_PKT_START). This 
will be calculated here. So when the timer interrupt occurs, the packet that initially started the timer will
be immediatly resent. Then, the next start time will be calculated based on the next packet that is unacked.
This next packet could be the packet which was just sent ( if there are no other unacked packets ) , which
the timer would then start for the base TIMEOUT time. Or, it could be another packet waiting. In that case,
the timer will then start for what ever time is remaining in that packets TIMEOUT.

For example - TIMEOUT = 30

Packet 0 will start timer , since it is the first in the window, or equal to base.

TIME  Event
0     Packet 0 Sent - This packet will drop
10    Packet 1 Sent - This packet will drop
15    Packet 2 Sent - This packet will drop

30    Timer Interrupt for packet 0
      Resend Packet 0
      Start Timer , this time calculate the time out based on the next packet waiting for ACK
      Packet = 1
      Sent Time = 10. Timer Interrupt time = 30.
      Timeout = TIMEOUT - (Interrupt Time - Sent Time) = 30 - (30 - 10) = 10
      Starttimer(10)

40    Timer Interrupt for packet 1
      Resend Packet 1
      Start Timer. Calculate time out based on next packet waiting for ACK
      Packet = 2. Sent Time = 15.
      Timeout = TIMEOUT - (Interrupt Time - Sent time) = 30 - (40 - 15) = 5
      Starttimer(5)

45    TimerInterrupt for packet 2





*/
void A_timerinterrupt(){

  float interrupt_time = get_sim_time();

  struct windowItem inter_pkt = getTimerStartWindowItem();
  

  /* Resend Packet */
  inter_pkt.timesent = get_sim_time();

  /* send to layer 3*/
  tolayer3(0,inter_pkt.p);

  /* Save Interrupting packet time time */
  float last_sent_time = inter_pkt.timesent;

  /* Update Packet in pkts_sent */
  pkts_sent[inter_pkt.p.seqnum] = inter_pkt; 

  /* Get time sent of next unacked packet this could be the one just re-sent or another */
  int last_sent_seqnum = inter_pkt.p.seqnum;

  /* Set next_packet_sent_time to packet just resent */
  float next_packet_sent_time = inter_pkt.timesent;


  /* Check if any packets are unacked after this that should set timer instead */
  struct windowItem start_timer_window_pkt = inter_pkt;

  for(int i = base; i <= base + pkts_unacked; i++){
    struct windowItem w = pkts_sent[i]; 
    if(w.p.acknum == -1 && w.p.seqnum != inter_pkt.p.seqnum){
      start_timer_window_pkt = w;
      next_packet_sent_time = w.timesent;

      /* Exit for loop this should grab the first one unacked after the packet */
      break;
    }

  }

  /* Calculate timer for next packet which is the next unacked packet */
  float diff = last_sent_time - next_packet_sent_time;
  float t = TIMEOUT - diff;
  updateTimerStartTime(t,start_timer_window_pkt);
  starttimer(0,t);
}  

/* the following routine will be called once (only) before any other */
/* entity A routines are called. You can use it to do any initialization */
void A_init()
{
  base = 0;                 /* Start base at 1 */
  nextseqnum = 0;           /* Start nextseqnum at 1 */
  N = getwinsize();         /* Set Window Size */
}

/* Note that with simplex transfer from a-to-B, there is no B_output() */
/* called from layer 3, when a packet arrives for layer 4 at B*/
void B_input(struct pkt recvd_packet)
{

  int checksum = 0;
  checksum = compute_checksum(recvd_packet);

  if(checksum == recvd_packet.checksum){

    /* Check if packet is between rcv_base and rcv_base + N */
    if(recvd_packet.seqnum >= rcv_base && recvd_packet.seqnum <= rcv_base + N - 1){

      /* Send ACK Packet to A*/
      struct pkt p;
      p.acknum = recvd_packet.seqnum;
      p.checksum = compute_checksum(p);
      tolayer3(1,p);

      if(recvd_packet.seqnum == rcv_base){

        /* Add to pkts recvd and mark as delivered */
        struct windowItem w;
        w.p = recvd_packet;
        w.delivered = 1;
        pkts_recvd[recvd_packet.seqnum] = w;

        /* Deliver to layer5 */
        tolayer5(1,w.p.payload);

        /* Deliver any buffered consecutive packets to layer5*/
        int diff = 1;
        int num_delivered = 1;

        for(int i = rcv_base; i < rcv_base + N; i++){
          struct windowItem w1 = pkts_recvd[i];
          struct windowItem w2 = pkts_recvd[i + 1];

          if(w2.p.seqnum - w1.p.seqnum == diff){

            /* Consecutive Packet - Deliver to layer 5 */
            tolayer5(1,w2.p.payload);
            pkts_recvd[i+1].delivered = 1;
            num_delivered++;
          }else{
            /* Found a non consecutive packet - exit loop */
            break;
          }
        }

        /* Increment rcv_base by number of packets delivered */
        rcv_base = rcv_base + num_delivered;
      }else{

        /* Buffer Packet */
        struct windowItem w;
        w.p = recvd_packet;
        w.delivered = 0;
        pkts_recvd[w.p.seqnum] = w;
      }
    }else if(recvd_packet.seqnum >= rcv_base - N && recvd_packet.seqnum <= rcv_base - 1){
      
      /* Send ACK for packet receieved - no need to buffer since it has already been recvd */
      struct pkt p;
      p.acknum = recvd_packet.seqnum; 
      p.checksum = compute_checksum(p);
      tolayer3(1,p); 
    }
  }else{
    /* Packet Corrupt - Let Timeout */
  }
}

/* the following rouytine will be called once (only) before any other */
/* entity B routines are called. You can use it to do any initialization */
void B_init()
{
  rcv_base = 0;
}
