# Reliable-Transport-Protocols
Demonstrates Alternating-Bit (ABT) , Go-Back-N (GBN) and Selective-Repeat (SR)

## Compilation  
Use provided makefile

## How to run  
All 3 programs take the same set of inputs. The main difference is that window size will not affect abt.
Arguments:  
-s Seed for random number generation  
-w Window size: Window size for SR and GBN. Abt does not use this but it is needed.
-m Number of messages to simulate  
-l Loss: % of packets that will be lossed
-c Corruption: % of packets that will be corrupted
-t Average time between messages from sender's layer5  
-v Tracing - these are print messages

Example  
./abt -s 1111 -w 10 -m 1000 -l 0.2 -c 0.1 -t 50 -v 0
