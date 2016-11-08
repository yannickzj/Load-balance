# Introduction
This is a mini project of a graduate course, which aims to simulate load balancing with nulti-producer/multi-consumer systems using the following approaches: 
+ threads and shared memory;
+ processes and message passing.

# Requirements
In this project, it is required to solve the multi-producer/multi-consumer problem with a bounded buffer using the above approaches. In the context of a typical software server, the producers are considered as being entities that are receivinig requests and consumers are thought of as entities that will process the request and generate a reply.

# Design choices

For simplicity’s sake, the poisson distribution is applied to producing the random variables of `Pt` , `Rs` , `Ct1` and `Ct2` in this assignment. Additionally, every producer sends one more character `MSGEND` at the end of each request, indicating the end of a complete request. In this way, the consumer can automatically realize the size of the request when it receives the `MSGEND`. Some more design details for the two approaches are described as follows:

## Multi-thread design

The group of producer threads produces data that is passed via a **queue** structure in the same process space to the group of consumer threads. In order to coordinate the producers and the consumers, two binary thread semaphores `sem_p` and `sem_c` are used to ensure that only one producer and one consumer can operate on the shared memory. Each time there are only a single producer sending a request and a single consumer receiving a request, it is not necessary to make use of the mutex lock. 

In current design, two extra semaphores, `empty` and `full` with initialization values equal to `0` and `B`, are used to control the buffer size so that the producer does not overfill the buffer and so that the consumer does not read data that has not been placed in the buffer by the producer.

## Multi-process design

As for the inter-process message passing approach, message queue facilities are used to establish the communication between the producer and the consumer. A process semaphore with two semaphore values (both are initially set to 1) is used to coordinate the producers and the consumers in the same way as the multi-thread communication. 

In order to control the buffer size, the message queue size is explicitly initialized with `B` bytes and set the msgflg with default value in the producer process such that the producer will be blocked if insufficient space is available in the message queue; On the other hand, the consumer uses `msgrcv()` to receive data without specifying `IPC_NOWAIT` in the msgflg, then the process will be automatically blocked if no message is available in the queue.

<p align="center"><img src="/README/blockedTime.png" width="700"></p>
![Alt text](/README/blockedTime.png)



<center>![equation](http://latex.codecogs.com/gif.latex? \\lambda_b=\\frac{P_r}{C_r}=\\frac{P/E(P_t)}{C/[p_iE(C_t1)+(1-p_i)E(C_t2)]})</center>


