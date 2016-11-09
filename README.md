# Load balancing simulation
This is a mini project of a graduate course, which aims to simulate load balancing with multi-producer/multi-consumer systems using the following approaches: 
+ threads and shared memory;
+ processes and message passing.

## Project Requirements
In this project, it is required to solve the multi-producer/multi-consumer problem with a bounded buffer using the above approaches. In the context of a typical software server, the producers are considered as being entities that are receivinig requests and consumers are thought of as entities that will process the request and generate a reply.

To emulate this, a produce function is defined for the producers to issue a new request for the consumers after a random delay `Pt`. The size of the request to be transmitted to the consumers is `Rs`, which is also randomly distributed. Similarly, a consume function is defined for the consumers to complete the task after a random time period. However, this time period is more complex, which is depending on whether the work requests need IO operations. Therefore, the paremeter `p_i` is defined as the probability that the work requires IO. There will be two randomly distributed consumer time values, `Ct1` and `Ct2`, where the first is chosen with probability `p_i` and otherwise the second is chosen. In addition, `B` is defined as the buffer size that producers may send without consumers having consumed them before producers must stop producing.

On a Linux platform, the execution command will follow the form as below:
```
./server_approachChoice <T> <B> <P> <C> <P_t parms> <R_s parms> <C_t1 parms> <C_t2 parms> <p_i>
```
where \<T> is the amount of time for which the system should execute, in sexonds, \<B> is a parameter specifying the size (in bytes) of the shared memory space/shared message-queue size, \<P> is the number of producers, \<C> is the number of consumers, \<P_t parms> is the parmeters related to the probabiity distribution for the random time *`Pt`* that the producers must wait between request productions, \<R_s parms> is the parameters related to the probability distribution of the request size, \<C_t1 parms> is the parmeters related to the probabiity distribution for the random time *`Ct1`* that the consumers take with probability *`p_i`*, \<C_t2 parms> is the parmeters related to the probabiity distribution for the random time *`Ct2`* that the consumers take with probability *`1-p_i`*, and \<p_i> is the probability *`p_i`*.

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

In order to better discuss the program results, a definition of **balance factor** ![equation](http://latex.codecogs.com/gif.latex? \\lambda_b) is given as follows:

<p align="center"><img src="/README/f1.png" width="300"></p>

The balance factor reflects the ratio of producing rate *`Pr`* to consuming rate *`Cr`*, where *`E()`* is expectation function of random variables. In current system, *`Pr`* and *`Cr`* are assumed to be proportional to the instance number (*`P`* or *`C`*) divided by the average processing time(*`E(Pr))`* or *`E(Ct1)p_i+(1-p_i)E(Ct2)`*). When *balance factor* is less than 1, it means the rate of processing requests is greater than that of generating and the consumers are averagely more faster than producers in terms of finishing their jobs; while as *balance factor* is greater than 1, the producers tend to be faster. Specifically, the program input parameters are chosen as follows:
```
T=120s; B=1024; P=6, 7, 8, 9, 10; C=15; Pt_parm=8ms;
Rs_parm=10; Ct1_parm=30ms; Ct2_parm=5ms; p_i=0.4
```
Thus, the balance factor can be derived from its definition:
```
balance factor=0.750, 0.875, 1.000, 1.125, 1.250, when P=6, 7, 8, 9, 10
```
Additionally, the steady state is chosen from T=\[10,110](s). All the average values in the following sessions are based on this time period.

### Average number of processed requests per second

When ![equation](http://latex.codecogs.com/gif.latex? \\lambda_b)\<1 , the average number of processed requests tends to be
proportional to the balance factor. While as the producer number keep increasing and the *balance factor* becomes equal to or greater than 1, the average processed request number converges to its maximum expectation, which can be calculated as follows:
>Assuming that producer blocked time is equal to 0 in the ideal situation, producers take turns to send requests into the buffer in steady state. Since delay time Pt for each producer is a poisson-distributed random variable. For a time interval of 1s, the maximum expected request number will be

<p align="center"><img src="/README/f2.png" width="200"></p>

Since communication via threads is generally more efficient than that via processes, it reasonable that multi-thread transmission can obtain greater processed request numbers per second than the multi-process approach.

<p align="center">
<br/><img src="/README/qnum.png" width="600">
<br/>Figure1. Average number of processed requests per second for both approaches
</p>

### Average producer blocked information

For both two approaches(Figure2), the producers are rarely blocked as the balance factor is less than 1.0. However, when ![equation](http://latex.codecogs.com/gif.latex? \\lambda_b) grows greater than 1.0, the producers are frequently blocked. Therefore, an equilibrium point can be observed when ![equation](http://latex.codecogs.com/gif.latex? \\lambda_b) =1.0, where the producers are blocked and the consumer are idle for few times and for reasonable little amount of time.

<p align="center">
<br/><img src="/README/blockedTime.png" width="600">
<br/>Figure2. Average producer blocked time(per second) for both approaches
</p>

In order to have a better understanding of this equilibrium point, the variation of request number in the buffer is observed and presented in Figure 3 and Figure 4. When the balance factor is equal to 1.0, the request quantity fluctuates between the upper and lower bound of the buffer size and seldom reaches the “top” or “bottom” of the buffer; But as the balance factor becomes greater than or less than 1.0, this equilibrium breaks and the request number in the buffer will frequently reach the buffer limits, resulting in blocked producers or idle consumers. This point can also be considered as the system full-load state because extra increase of request rate will lead to significant increase of blocked producers.

<p align="center">
<br/><img src="/README/timehistory_shm.png" width="600">
<br/>Figure3. Time-history variation of request number in the buffer for multi-thread approach
<br/>(where lambda_b in the legend is the balance factor)
</p>

<p align="center">
<br/><img src="/README/timehistory_msq.png" width="600">
<br/>Figure4. Time-history variation of request number in the buffer for multi-process approach
<br/>(where lambda_b in the legend is the balance factor)
</p>

## Conclusions

The ideal number of producers and consumers can be determined when the balance factor ![equation](http://latex.codecogs.com/gif.latex? \\lambda_b) reach 1.0. At this point, the system can reach an equilibrium full-load state, where the the request quantity in the buffer fluctuates between the upper and lower bound of the buffer size and seldom reaches the “top” or “bottom” of the buffer. Therefore, most number of requests can be processed with fewest blocked producers and idle consumers.

