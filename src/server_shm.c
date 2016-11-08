#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define MSGEND '!'
#define TIMEINTERVAL 1

//transmission state variables
int requestSnd;
int requestPrc;
int numBlocked;
int numIdle;
float timeBlocked;
float timeIdle;

//semaphores to coodinate producers and consumers 
sem_t sem_p;
sem_t sem_c;

//'empty' semaphore counting the number of occupied slots in the queue
//'full' semaphore counting the number of available slots in the queue 
sem_t empty;
sem_t full;

//queue structure
struct queue {
	char* q_array;
	int head;
	int tail;
	int size;
};


//producer parameter structure
struct producer_parms {
	int Pt_parm;
	int Rs_parm;
	struct queue* shared_queue;     
};

//consumer parameter structure
struct consumer_parms {
	int Ct1_parm;
	int Ct2_parm;
	float p_i;
	struct queue* shared_queue;     
};

//produce [0,1] random numbers
double uRandom()   
{
	return rand()*1.0/RAND_MAX;
}

//produce poisson distrubuted random numbers
int poisson(int lambda)  
{
	if (lambda>100) {
		fprintf(stderr, "lambda is too large\n");
		return lambda;
	}
    int k = 0;
    long double p = 1.0;
	long double l=exp(-lambda);  
    while (p>=l)
    {
	    double u = uRandom();
	    p *= u;
	    k++;
	}
    return k-1;
}

//function to get integer random number
int randomInt (int parm) {
	//return rand()%parm+1;
	return poisson(parm);
} 

//function to produce random char
char randChar() {
	return 'a'+rand()%26;
}

//function to  process request
void request_process (int Ct1_parm, int Ct2_parm, float p_i) {
	float t = uRandom();
	int process_time;
	if (t<p_i) {
		process_time = randomInt(Ct1_parm);
	} else {
		process_time = randomInt(Ct2_parm);	
	}
	usleep (process_time*1000);
}

//producer thread
void* producer(void* parameters) {

	//get input parameters
	struct producer_parms* args = (struct producer_parms*) parameters;
	int Pt_parm = args->Pt_parm;
	int Rs_parm = args->Rs_parm;
	struct queue* shared_queue = args->shared_queue;    
	
	//producer parameters
	int flg_blk = 0;
	int semValue;
	struct timeval t1;
	struct timeval t2;

	while (1) {
		//produce P_t and R_s
		int P_t = randomInt(Pt_parm);
		int R_s = randomInt(Rs_parm);

		//delay P_t
		usleep(P_t*1000);

	
		//wait for sem_p
		sem_wait(&sem_p);
		
		//put request in the shared queue
		int sndCount = 0;
		while (sndCount<R_s) {

			//check whether the buffer is full
			sem_getvalue(&full, &semValue); 
			if (semValue==0) {
				flg_blk = 1;
				numBlocked++;
				gettimeofday(&t1, NULL);
			}

			sem_wait (&full);

			//calculate the blocked time
			if (flg_blk) {
				flg_blk = 0;
				gettimeofday(&t2, NULL);
				timeBlocked += (t2.tv_sec + t2.tv_usec/1000000.0) - (t1.tv_sec + t1.tv_usec/1000000.0);
			}

			shared_queue->q_array[shared_queue->tail] = randChar();
			if (shared_queue->tail==shared_queue->size-1) {
				shared_queue->tail = 0;
			} else {
				shared_queue->tail++;
			}
			sem_post (&empty);
			sndCount++;
		}

		//put an ending sigal in the shared queue
		sem_wait (&full);
		shared_queue->q_array[shared_queue->tail] = MSGEND;
		if (shared_queue->tail==shared_queue->size-1) {
			shared_queue->tail = 0;
		} else {
			shared_queue->tail++;
		}
		sem_post (&empty);

		requestSnd++;

		pthread_testcancel();

		//post sem_p
		sem_post(&sem_p);
	}

	return NULL;
}

//consumer thread
void* consumer(void* parameters) {

	//get input parameters
	struct consumer_parms* args = (struct consumer_parms*) parameters;
	int Ct1_parm = args->Ct1_parm;
	int Ct2_parm = args->Ct2_parm;
	float p_i = args->p_i;
	struct queue* shared_queue = args->shared_queue;    

	//consumer parameters
	int flg_idle = 0;
	int semValue;
	struct timeval t1;
	struct timeval t2;

	while (1) {
		//wait for sem_c
		sem_wait(&sem_c);

		//receive request from the shared queue
		int message = 0;
		while (message!=MSGEND) {

			//check whether the buffer is empty
			sem_getvalue(&empty, &semValue); 
			if (semValue==0) {
				flg_idle = 1;
				numIdle++;
				gettimeofday(&t1, NULL);
			}

			sem_wait (&empty);

			//calculate the idle time
			if (flg_idle) {
				flg_idle = 0;
				gettimeofday(&t2, NULL);
				timeIdle += (t2.tv_sec + t2.tv_usec/1000000.0) - (t1.tv_sec + t1.tv_usec/1000000.0);
			}

			message = shared_queue->q_array[shared_queue->head];
			if (shared_queue->head==shared_queue->size-1) {
				shared_queue->head= 0;
			} else {
				shared_queue->head++;
			}
			sem_post (&full);
		}

		pthread_testcancel();

		//post sem_c
		sem_post (&sem_c);

		//process request
		request_process (Ct1_parm, Ct2_parm, p_i);
		requestPrc++;
	}
	return NULL;
}


int main(int argc, char* argv[]) {
	
	//timer initialization
	struct timeval tv1;
	struct timeval tv2;
	struct timeval tv3;
	srand((unsigned) time(NULL));	
	
	//set timer1
	gettimeofday(&tv1, NULL);

	//input check
	if (argc != 10) {
		fprintf(stderr, ("Input error!\n"));
		exit(1);
	}

	int T = atof(argv[1]);
	int B = atoi(argv[2]);
	int P = atoi(argv[3]);
	int C = atoi(argv[4]);
	int Pt_parm = atof(argv[5]);
	int Rs_parm = atoi(argv[6]);
	int Ct1_parm = atof(argv[7]);
	int Ct2_parm = atof(argv[8]);
	float p_i = atof(argv[9]);

	requestPrc = 0;
	requestSnd = 0;
	
	//semaphore initialization
	sem_init (&empty, 0, 0);
	sem_init (&full, 0, B);
	sem_init (&sem_c, 0, 1);
	sem_init (&sem_p, 0, 1);

	//thread parameter initialization
	struct queue shared_queue;
	shared_queue.q_array = (char*) malloc(sizeof(char)*B);
	shared_queue.tail = 0;
	shared_queue.head = 0;
	shared_queue.size = B;

	struct producer_parms producer_args;
	producer_args.Pt_parm = Pt_parm;
	producer_args.Rs_parm = Rs_parm;
	producer_args.shared_queue = &shared_queue;

	struct consumer_parms consumer_args;
	consumer_args.Ct1_parm = Ct1_parm;
	consumer_args.Ct2_parm = Ct2_parm;
	consumer_args.p_i = p_i;
	consumer_args.shared_queue = &shared_queue;

	//producer and consumer threads declaration
	pthread_t list_p[P];
	pthread_t list_c[C];

	//set timer2
	gettimeofday(&tv2, NULL);

	//begin to transfer data
	int i;
	for (i=0; i<P; i++) {
		if (pthread_create (&list_p[i], NULL, &producer, &producer_args)!=0) {
			fprintf(stderr, "producer pthread_create error!\n");
		}
	}
	for (i=0; i<C; i++) {
		if (pthread_create (&list_c[i], NULL, &consumer, &consumer_args)!=0) {
			fprintf(stderr, "consumer pthread_create error!\n");
		}
	}

	//set timer3
	gettimeofday(&tv3, NULL);

	//print time differences
	double t1 = tv1.tv_usec/1000000.0 + tv1.tv_sec;
	double t2 = tv2.tv_usec/1000000.0 + tv2.tv_sec;
	double t3 = tv3.tv_usec/1000000.0 + tv3.tv_sec;
	double dt1;
	double dt2;
	dt1 = t2-t1;
	dt2 = t3-t2;


	//wait for cetain execution time and display processing info
	printf("\n");
	int numSnd0 = 0;
	int numSnd1 = 0;
	int numPrc0 = 0;
	int numPrc1 = 0;
	int numBlk0 = 0;
	int numBlk1 = 0;
	int numIdle0 = 0;
	int numIdle1 = 0;
	float timeBlk0 = 0.0;
	float timeBlk1 = 0.0;
	float timeIdle0 = 0.0;
	float timeIdle1 = 0.0;
	int time=0;
	int numQueue;

	//store time differences in local file
	FILE *fp;
	char *fileName = (char*) "output_shm.txt";
	if ((fp=fopen(fileName, "a+"))==NULL) {
		fprintf(stderr, ("cannot open file!\n"));
		exit(1);
	}
	fprintf(fp, "T=%d, B=%d, P=%d, C=%d, Pt_parm=%d, Rs_parm=%d, Ct1_parm=%d, Ct2_parm=%d, p_i=%f\n", T, B, P, C, Pt_parm, Rs_parm, Ct1_parm, Ct2_parm, p_i);
	fprintf(fp, "initialization_time=%.6fs, pthread_time=%.6fs\n", dt1, dt2);
	fprintf(fp, "%-15s%-15s%-15s%-15s%-15s%-15s%-15s%-15s\n", "T", "occupied", "empty", "numPrc", "numBlk", "timeBlk(s)", "numIdle", "timeIdle(s)");

	while (time<T) {
		sleep(TIMEINTERVAL);
		time += TIMEINTERVAL;
		numSnd1 = requestSnd;
		numPrc1 = requestPrc;
		numBlk1 = numBlocked;
		numIdle1 = numIdle;
		timeBlk1 = timeBlocked;	
		timeIdle1 = timeIdle;	
		sem_getvalue(&empty, &numQueue); 
		fprintf(fp, "%-15d%-15d%-15d%-15d%-15d%-15.6f%-15d%-15.6f\n", time, numQueue, B-numQueue, numPrc1-numPrc0, numBlk1-numBlk0, timeBlk1-timeBlk0, numIdle1-numIdle0, timeIdle1-timeIdle0);
		numSnd0 = requestSnd;
		numPrc0 = requestPrc;
		numBlk0 = numBlocked;
		numIdle0 = numIdle;
		timeBlk0 = timeBlocked;	
		timeIdle0 = timeIdle;
	}

	fprintf(fp, "\n");
	fclose(fp);

	//cancel the producer and consumer threads
	for (i=0; i<P; i++) {
		if (pthread_cancel(list_p[i])!=0) {
			fprintf(stderr, "producer pthread_cancel error!\n");
		}
	}

	for (i=0; i<C; i++) {
		if (pthread_cancel(list_c[i])!=0) {
			fprintf(stderr, "consumer pthread_cancel error!\n");
		}
	}

	return 0;
}
