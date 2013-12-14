Distributed programming - coursework
====================================

A process (coordinator) creates 5 processes (pool). 
The workers wait to be wake from the coordinator, do some work and wait again.
The coordinator send a marker (integer) to the first process and wake it up.
The worker adds one to the marker and send it to next process while notify the coordinator.
The coordinator wake up the next worker and so on.
The marker should pass each process 10 times, after that the coordinator prints it.
For synchronization one must use signaling, for communication one must use pipes.

Alternative solution with threads should be provided.
