# Reliable-Transport-Protocols
Demonstrates Alternating-Bit (ABT) , Go-Back-N (GBN) and Selective-Repeat (SR)

## Compilation  
Use provided makefile

## How to run  
All 3 programs take the same set of inputs. The main difference is that window size will not affect abt.

ARGS: -s Seed -w Window size -m Number of messages to simulate -l Loss -c Corruption -t Average time between messages from sender's layer5 -v Tracing
