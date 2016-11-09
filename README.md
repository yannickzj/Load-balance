# Load balancing simulation
This is a mini project of a graduate course, which aims to simulate load balancing with nulti-producer/multi-consumer systems using the following approaches: 
+ threads and shared memory;
+ processes and message passing.

## Project Requirements
In this project, it is required to solve the multi-producer/multi-consumer problem with a bounded buffer using the above approaches. In the context of a typical software server, the producers are considered as being entities that are receivinig requests and consumers are thought of as entities that will process the request and generate a reply.

To emulate this, a produce function is defined for the producers to issue a new request for the consumers after a random delay `Pt`. The size of the request to be transmitted to the consumers is `Rs`, which is also randomly distributed. Similarly, a consume function is defined for the consumers to complete the task after a random time period. However, this time period is more complex, which is depending on whether the work requests need IO operations. Therefore, the paremeter `pi` is defined as the probability that the work requires IO. There will be two randomly distributed consumer time values, `Ct1` and `Ct2`, where the first is chosen with probability `pi` and otherwise the second is chosen. In addition, `B` is defined as the buffer size that producers may send without consumers having consumed them before producers must stop producing.

Finally, for given parameters, the ideal number of producers and consumers needs to be determined. The ideal will be that number for which the most number of requests are processed, with fewest blocked producers.



## Design choices

For simplicity’s sake, the poisson distribution is applied to producing the random variables of `Pt` , `Rs` , `Ct1` and `Ct2` in this project. Additionally, every producer sends one more character `MSGEND` at the end of each request, indicating the end of a complete request. In this way, the consumer can automatically realize the size of the request when it receives the `MSGEND`. Some more design details for the two approaches are described as follows:

### 1. Multi-thread design

The group of producer threads produces data that is passed via a **queue** structure in the same process space to the group of consumer threads. In order to coordinate the producers and the consumers, two binary thread semaphores `sem_p` and `sem_c` are used to ensure that only one producer and one consumer can operate on the shared memory. Each time there are only a single producer sending a request and a single consumer receiving a request, it is not necessary to make use of the mutex lock. 

In current design, two extra semaphores, `empty` and `full` with initialization values equal to `0` and `B`, are used to control the buffer size so that the producer does not overfill the buffer and so that the consumer does not read data that has not been placed in the buffer by the producer.

### 2. Multi-process design

As for the inter-process message passing approach, message queue facilities are used to establish the communication between the producer and the consumer. A process semaphore with two semaphore values (both are initially set to 1) is used to coordinate the producers and the consumers in the same way as the multi-thread communication. 

In order to control the buffer size, the message queue size is explicitly initialized with `B` bytes and set the `msgflg` with default value in the producer process such that the producer will be blocked if insufficient space is available in the message queue; On the other hand, the consumer uses `msgrcv()` to receive data without specifying `IPC_NOWAIT` in the msgflg, then the process will be automatically blocked if no message is available in the queue.

## Interesting Observations

![equation](http://latex.codecogs.com/gif.latex? \\lambda_b=\\frac{P_r}{C_r}=\\frac{P/E(P_t)}{C/[p_iE(C_t1)+(1-p_i)E(C_t2)]})


<p align="center"><img src="/README/blockedTime.png" width="700"></p>

<p align="center"> Figure1. blabla </p>



|2000|2010|
|----|----|
|50|30|

